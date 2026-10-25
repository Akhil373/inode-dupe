#ifndef TREE_H
#define TREE_H

#include <sys/stat.h>
typedef void (*file_cb)(const char* path, struct stat*, void* ctx);

int walk_dir(const char* root, file_cb cb, void* ctx);

#endif
