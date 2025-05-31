#include "memory.h"
#include <gc.h>
#include <string.h>
#include <stdlib.h> // for malloc/free (temp arena fallback)

void ar_memory_init() {
    GC_INIT();
}

void* ar_alloc(size_t size) {
    return GC_MALLOC(size);
}

void* ar_alloc_atomic(size_t size) {
    return GC_MALLOC_ATOMIC(size);
}

char* ar_strdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* copy = (char*)GC_MALLOC(len);
    memcpy(copy, str, len);
    return copy;
}