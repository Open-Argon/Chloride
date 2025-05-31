#ifndef ARGON_MEMORY_H
#define ARGON_MEMORY_H

#include <stddef.h> // for size_t

// GC-managed allocations
void* ar_alloc(size_t size);
void* ar_alloc_atomic(size_t size);
char* ar_strdup(const char* str);

// Memory init/shutdown
void ar_memory_init();

#endif // ARGON_MEMORY_H