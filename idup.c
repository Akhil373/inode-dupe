#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "tree.h"
#include "xxhash.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

const int CHUNK_SIZE = 65536;

struct file {
    char* path;
    off_t size;
    dev_t dev;
    ino_t ino;
    uint64_t hash;
    int group_id;
};

struct file_list {
    struct file* items;
    size_t count;
    size_t cap;
    int next_group_id;
};

static int compare_ints(const void* a, const void* b) {
    const struct file* fa = a;
    const struct file* fb = b;
    if (fa->size < fb->size) return -1;
    if (fa->size > fb->size) return 1;
    return 0;
}

static int cmp_by_hash(const void* a, const void* b) {
    const struct file *fa = a, *fb = b;
    return (fa->hash > fb->hash) - (fa->hash < fb->hash);
}

static void retrieve_files(const char* path, struct stat* fileStat, void* ctx) {
    struct file_list* fl = ctx;
    if (fl->count == fl->cap) {
        size_t newcap = fl->cap * 2;
        struct file* tmp = realloc(fl->items, newcap * sizeof *tmp);
        if (!tmp) {
            perror("relloc");
            return;
        }
        fl->items = tmp;
        fl->cap = newcap;
    }

    fl->items[fl->count].path = strdup(path);
    fl->items[fl->count].dev = fileStat->st_dev;
    fl->items[fl->count].ino = fileStat->st_ino;
    fl->items[fl->count].size = fileStat->st_size;
    fl->items[fl->count].group_id = -1;
    fl->count++;
}

static void drop_unique_files(struct file_list* fl) {
    size_t r = 0, w = 0;
    while (r < fl->count) {
        size_t i = r + 1;
        while (i < fl->count && fl->items[i].size == fl->items[r].size) i++;
        if (i - r > 1) {
            for (size_t k = r; k < i; k++) {
                fl->items[w++] = fl->items[k];
            }
        } else {
            free(fl->items[r].path);
        }
        r = i;
    }
    fl->count = w;
}

static void mem_cmp(struct file_list* fl, size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
        if (fl->items[i].group_id >= 0 || fl->items[i].path == NULL) continue;

        FILE* f1 = fopen(fl->items[i].path, "rb");
        if (!f1) {
            printf("fopen error in memcmp: skipping file %s\n",
                   fl->items[i].path);
            free(fl->items[i].path);
            fl->items[i].path = NULL;
            continue;
        }

        fl->items[i].group_id = fl->next_group_id++;

        size_t bytes_read1;
        char buf1[CHUNK_SIZE];

        for (size_t j = i + 1; j < end; ++j) {
            if (fl->items[j].path == NULL || fl->items[j].group_id >= 0)
                continue;
            FILE* f2 = fopen(fl->items[j].path, "rb");
            if (!f2) {
                printf("fopen error in memcmp: skipping file %s\n",
                       fl->items[j].path);
                free(fl->items[j].path);
                fl->items[j].path = NULL;
                continue;
            }
            rewind(f1);

            char buf2[CHUNK_SIZE];
            size_t bytes_read2;
            int result = 0;
            while ((bytes_read1 = fread(buf1, 1, CHUNK_SIZE, f1)) > 0) {
                bytes_read2 = fread(buf2, 1, CHUNK_SIZE, f2);
                if (bytes_read1 != bytes_read2) {
                    result = 1;
                    break;
                }

                int cmp = memcmp(buf1, buf2, bytes_read1);
                if (cmp != 0) {
                    result = cmp;
                    break;
                }
            }
            if (result == 0 && fread(buf2, 1, CHUNK_SIZE, f2) > 0) result = 1;
            fclose(f2);
            if (result == 0) {
                fl->items[j].group_id = fl->items[i].group_id;
            }
        }
        fclose(f1);
    }
}

static void partial_hash(struct file_list* fl, size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
        if (fl->items[i].group_id >= 0 || fl->items[i].path == NULL) continue;
        FILE* f1 = fopen(fl->items[i].path, "rb");
        if (!f1) {
            printf("skipping %s...\n", fl->items[i].path);
            free(fl->items[i].path);
            fl->items[i].path = NULL;
            fl->items[i].hash = 0;
            continue;
        }
        size_t len = MIN(CHUNK_SIZE, fl->items[i].size);
        unsigned char buf[len];
        size_t n1 = fread(buf, 1, sizeof buf, f1);
        fseek(f1, len / 2, SEEK_SET);

        XXH3_state_t* st = XXH3_createState();
        if (!st) {
            fclose(f1);
            continue;
        }

        XXH3_64bits_reset(st);
        XXH3_64bits_update(st, buf, n1);

        size_t n2 = fread(buf, 1, sizeof buf, f1);
        XXH3_64bits_update(st, buf, n2);

        uint64_t hsh = XXH3_64bits_digest(st);
        XXH3_freeState(st);
        fl->items[i].hash = hsh;

        fclose(f1);
    }

    qsort(&fl->items[begin], end - begin, sizeof(struct file), cmp_by_hash);

    size_t r = begin, w = begin;
    while (r < end) {
        size_t i = r + 1;
        while (i < end && fl->items[i].hash == fl->items[r].hash) i++;
        if (i - r == 1 && fl->items[i].path) {
            free(fl->items[r].path);
            fl->items[r].path = NULL;
        } else
            mem_cmp(fl, r, i);
        r = i;
    }
}

static void compare_files(struct file_list* fl) {
    size_t n = fl->count, i = 0;
    while (i < n) {
        size_t curr_size = fl->items[i].size;
        size_t end = i;
        while (end < n && fl->items[end].size == curr_size) end++;
        if (end - i == 1) {
            free(fl->items[i].path);
            fl->items[i].path = NULL;
        }
        if (end - i == 2)
            mem_cmp(fl, i, end);
        else
            partial_hash(fl, i, end);
        i = end;
    }

    size_t w = 0;
    for (size_t i = 0; i < n; ++i) {
        if (fl->items[i].path == NULL) continue;
        if (w != i) fl->items[w] = fl->items[i];
        w++;
    }
    fl->count = w;
}

static void report_duplicates(struct file_list* fl) {
    size_t total_groups = 0, total_redundant = 0;

    for (size_t i = 0; i < fl->count;) {
        int group_id = fl->items[i].group_id;

        size_t end = i + 1;
        while (end < fl->count && fl->items[end].group_id == group_id) end++;
        size_t group_size = end - i;

        if (group_id >= 0 && group_size >= 2) {
            total_groups++;
            total_redundant += group_size - 1;

            printf("\nGroup %zu (%zu identical files)\n", total_groups,
                   group_size);
            for (size_t j = i; j < end; ++j)
                printf("  %s %s\n", j == i ? "[keep?]" : "[dup]  ",
                       fl->items[j].path);
        }
        i = end;
    }

    if (total_groups == 0) {
        puts("No duplicates found.");
        return;
    }

    printf("\nSummary: %zu duplicate group(s), %zu redundant file(s)\n",
           total_groups, total_redundant);
}

int main(int argc, char** argv) {
    int arg_counter = 1;

    struct file_list files;
    files.count = 0;
    files.cap = 1024;
    files.items = malloc(files.cap * sizeof(struct file));
    files.next_group_id = 0;

    if (argc == 1) walk_dir(".", retrieve_files, &files);

    char* seen;

    while (++arg_counter <= argc) {
        printf("\nListing %s...\n", argv[arg_counter - 1]);
        walk_dir(argv[arg_counter - 1], retrieve_files, &files);
    }

    // 1. sort
    // 2. drop unqiue sized file
    // 3. compare with partial hash and verify with memcmp
    // 5. print duplicates (if any)
    qsort(files.items, files.count, sizeof(struct file), compare_ints);
    drop_unique_files(&files);
    compare_files(&files);
    report_duplicates(&files);

    for (size_t i = 0; i < files.count; ++i) free(files.items[i].path);
    free(files.items);

    return 0;
}
