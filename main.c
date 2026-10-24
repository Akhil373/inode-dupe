#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int counter = 0;
const int BOUND = 512;
int capacity = 128;

int listdir(const char* path) {
    struct dirent* entry;
    DIR* dp;

    dp = opendir(path);
    if (dp == NULL) {
        perror("opendir");
        return -1;
    }

    char** subpaths = malloc(capacity * sizeof(char*));
    int count = 0;
    if (subpaths == NULL) {
        perror("malloc");
        return -1;
    }
    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (count >= capacity) {
            capacity = (capacity * 2 > BOUND) ? BOUND : capacity * 2;
            char** tmp = realloc(subpaths, capacity * sizeof(char*));
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
        char tmp_path[1024];
        // if (abs_path == NULL) perror("malloc");

        sprintf(tmp_path, "%s/%s", path, filename);
        char* abs_path = realpath(tmp_path, NULL);

        if (abs_path != NULL) {
            if (stat(abs_path, &fileStat) == 0) {
                if (S_ISDIR(fileStat.st_mode)) {
                    printf("+ %*s%s/\n", counter * 4, "", filename);
                    ++counter;
                    listdir(abs_path);
                    --counter;
                } else {
                    printf("%*s- %s\n", counter * 4, "", filename);
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

int main(int argc, char** argv) {
    int counter = 1;

    if (argc == 1) listdir(".");

    char* seen;

    while (++counter <= argc) {
        printf("\nListing %s...\n", argv[counter - 1]);
        listdir(argv[counter - 1]);
    }

    return 0;
}
