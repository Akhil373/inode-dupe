#include "tree.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int counter = 0;
static int capacity = 512;

static const char* ignorelist[] = {
    ".",      "..",     ".git",  ".svn",    ".hg",         "node_modules",
    "vendor", "target", "dist",  "build",   "__pycache__", ".venv",
    "venv",   ".tox",   ".idea", ".vscode", ".cache"};

static int walk_impl(const char* path, file_cb cb, void* ctx) {
    struct dirent* entry;
    DIR* dp;

    int rc = 0;

    dp = opendir(path);
    if (dp == NULL) {
        perror("opendir");
        return -1;
    }

    char** subpaths = malloc(capacity * sizeof(char*));
    int count = 0;
    if (subpaths == NULL) {
        perror("malloc");
        closedir(dp);
        return -1;
    }

    size_t ignorelist_len = sizeof(ignorelist) / sizeof(*ignorelist);
    while ((entry = readdir(dp)) != NULL) {
        int skip = 0;
        for (int x = 0; x < ignorelist_len; ++x) {
            if (strcmp(entry->d_name, ignorelist[x]) == 0) {
                skip = 1;
                break;
            }
        }
        if (skip) continue;
        if (count >= capacity) {
            capacity <<= 1;
            char** tmp = realloc(subpaths, capacity * sizeof(char*));
            if (tmp == NULL) {
                perror("realloc");
                for (int i = 0; i < count; ++i) free(subpaths[i]);
                free(subpaths);
                closedir(dp);
                return -1;
            }
            subpaths = tmp;
        }
        subpaths[count] = malloc(strlen(entry->d_name) + 1);
        if (subpaths[count] != NULL) {
            strcpy(subpaths[count], entry->d_name);
            count++;
        }
    }
    closedir(dp);

    struct stat fileStat;
    for (int i = 0; i < count; ++i) {
        char* filename = subpaths[i];
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;
        char tmp_path[2048];
        // if (abs_path == NULL) perror("malloc");

        int expected_len =
            snprintf(tmp_path, sizeof(tmp_path), "%s/%s", path, filename);

        if (expected_len < 0) {
            perror("snprintf encoding error");
        } else if (expected_len >= 1024) {
            printf("path too long\n");
            return -1;
        }
        char* abs_path = realpath(tmp_path, NULL);

        if (abs_path != NULL) {
            if (lstat(abs_path, &fileStat) == 0) {
                if (S_ISDIR(fileStat.st_mode)) {
                    // printf("+ %*s%s/\n", counter * 4, "", filename);
                    ++counter;
                    // listdir(abs_path);
                    rc = walk_impl(abs_path, cb, ctx);
                    --counter;
                } else if (S_ISREG(fileStat.st_mode)) {
                    // printf("%*s- %s\n", counter * 4, "", filename);
                    cb(abs_path, &fileStat, ctx);
                }
            }
        } else {
            perror("realpath");
        };
        free(abs_path);
        free(subpaths[i]);
    }
    free(subpaths);

    return 0;
}

int walk_dir(const char* root, file_cb cb, void* ctx) {
    return walk_impl(root, cb, ctx);
}
