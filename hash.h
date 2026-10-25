#ifndef HASH_H
#define HASH_H
#include <stddef.h>
#include <stdint.h>

#include "xxhash.h"

uint64_t hash_buffer(const void* buf, size_t size);
int hash_file(const char* path, uint64_t* out);

#endif
