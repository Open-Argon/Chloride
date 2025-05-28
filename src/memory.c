#include "memory.h"
#include <gc.h>
#include <string.h>
#include <stdlib.h> // for malloc/free (temp arena fallback)

static char* temp_arena = NULL;
static size_t temp_arena_capacity = 0;
static size_t temp_arena_offset = 0;

#define TEMP_ARENA_INITIAL_CAPACITY 4096

void ar_memory_init() {
    GC_INIT();
}

void ar_memory_shutdown() {
    // No-op for Boehm, but could clean up temp arena here
    if (temp_arena) {
        free(temp_arena);
        temp_arena = NULL;
        temp_arena_capacity = 0;
        temp_arena_offset = 0;
    }
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