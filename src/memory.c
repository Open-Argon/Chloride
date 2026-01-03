/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "memory.h"
#include <gc.h>
#include <pthread.h>
#include <stddef.h>
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

void *checked_realloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    fprintf(stderr, "fatal error: failed to allocate %zu bytes\n", size);
    exit(EXIT_FAILURE);
  }
  return new_ptr;
}

struct allocation *memory_allocations = NULL;
size_t memory_allocations_size = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void ar_memory_init() {
  GC_INIT();
  // memory_allocations_size = 8;
  // memory_allocations = malloc(memory_allocations_size*sizeof(struct
  // allocation));
}

void ar_memory_shutdown() {
  // for (size_t i = 0; i<memory_allocations_size;i++) {
  //   if (memory_allocations[i].status != allocation_fully_freed) {
  //     free(memory_allocations[i].ptr);
  //   }
  // }
  // free(memory_allocations);
}

void *ar_alloc(size_t size) {
  void *ptr = GC_MALLOC(size);
  if (!ptr) {
    fprintf(stderr, "panic: unable to allocate memory\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *ar_realloc(void *old, size_t size) {
  void *ptr = GC_REALLOC(old, size);
  if (!ptr) {
    fprintf(stderr, "panic: unable to allocate memory\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

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