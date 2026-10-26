#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "xxhash.h"

XXH64_hash_t hash_buffer(const void* buffer, size_t size) {
    return XXH3_64bits(buffer, size);
}
