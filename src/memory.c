/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "memory.h"
#include <gc.h>
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h> // for malloc/free (temp arena fallback)
#include <string.h>

void *checked_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "fatal error: failed to allocate %zu bytes\n", size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *gmp_gc_realloc(void *ptr, size_t old_size, size_t new_size) {
  (void)old_size; // Ignore old_size, Boehm doesn't need it
  return GC_realloc(ptr, new_size);
}

void gmp_gc_free(void *ptr, size_t size) {
  (void)size; // Boehm GC manages this itself
  // No-op — memory will be collected automatically
  GC_FREE(ptr);
}

void ar_memory_init() {
  GC_INIT();
}

void *ar_alloc(size_t size) { return GC_MALLOC(size); }

void *ar_realloc(void *old, size_t size) { return GC_REALLOC(old, size); }

void ar_finalizer(void *obj, GC_finalization_proc fn, void *client_data,
                  GC_finalization_proc *old_fn, void **old_client_data) {
  return GC_register_finalizer_no_order(obj, fn, client_data, old_fn,
                                        old_client_data);
}

void *ar_alloc_atomic(size_t size) { return GC_MALLOC_ATOMIC(size); }

char *ar_strdup(const char *str) {
  size_t len = strlen(str) + 1;
  char *copy = (char *)GC_MALLOC(len);
  memcpy(copy, str, len);
  return copy;
}