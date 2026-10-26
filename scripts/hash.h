#ifndef HASH_H
#define HASH_H
#include <stddef.h>
#include <stdint.h>

#include "xxhash.h"

XXH64_hash_t hash_buffer(const void* buf, size_t size);

#endif
