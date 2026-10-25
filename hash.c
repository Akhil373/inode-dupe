#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "xxhash.h"

XXH64_hash_t hash_buffer(const void* buffer, size_t size) {
    return XXH3_64bits(buffer, size);
}

int hash_prefix(const char* path, size_t nbytes, uint64_t* out) {
    unsigned char buf[65536];
    size_t want = nbytes < sizeof buf ? nbytes : sizeof buf;
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(buf, 1, want, f);
    fclose(f);
    *out = XXH3_64bits(buf, n);
    return 0;
}
