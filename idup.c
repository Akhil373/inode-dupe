#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "tree.h"

struct file {
    char* path;
    off_t size;
    dev_t dev;
    ino_t ino;
};

struct file_list {
    struct file* items;
    size_t count;
    size_t cap;
};

static int compare_ints(const void* a, const void* b) {
    const struct file* fa = a;
    const struct file* fb = b;
    if (fa->size < fb->size) return -1;
    if (fa->size > fb->size) return 1;
    return 0;
}

static void retrieve_files(const char* path, struct stat* fileStat, void* ctx) {
    struct file_list* fl = ctx;
    if (fl->count == fl->cap) {
        fl->cap <<= 1;
        struct file* tmp = realloc(fl->items, fl->cap);
        if (!tmp) {
            perror("relloc");
            return;
        }
        fl->items = tmp;
    }

    fl->items[fl->count].path = strdup(path);
    fl->items[fl->count].dev = fileStat->st_dev;
    fl->items[fl->count].ino = fileStat->st_ino;
    fl->items[fl->count].size = fileStat->st_size;
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

int main(int argc, char** argv) {
    int arg_counter = 1;

    struct file_list files;
    files.count = 0;
    files.cap = 1024;
    files.items = malloc(files.cap * sizeof(struct file));

    if (argc == 1) walk_dir(".", retrieve_files, &files);

    char* seen;

    while (++arg_counter <= argc) {
        printf("\nListing %s...\n", argv[arg_counter - 1]);
        walk_dir(argv[arg_counter - 1], retrieve_files, &files);
    }

    // 1. sort
    // 2. drop unqiue sized file
    // 3. compare partial hash
    // 4. match w/ sha256 full hash
    // 5. (maybe) verify by comparing every single byte

    qsort(files.items, files.count, sizeof(struct file), compare_ints);
    drop_unique_files(&files);

    for (int i = 0; i < files.count; ++i) {
        printf("%ld -> %s\n", files.items[i].size, files.items[i].path);
    }

    for (size_t i = 0; i < files.count; ++i) free(files.items[i].path);
    free(files.items);

    return 0;
}
