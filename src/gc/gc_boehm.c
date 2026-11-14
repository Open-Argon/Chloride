/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Boehm-Demers-Weiser GC backend. The default unless built with GC=custom.

#ifndef ARGON_GC_CUSTOM

#define GC_THREADS
#include "../memory.h"
#include <gc/gc.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static void *oom(const char *what, size_t size) {
  fprintf(stderr, "panic: unable to %s memory: %zu\n", what, size);
  exit(EXIT_FAILURE);
}

void ar_memory_init(void) {
  GC_INIT();
  GC_allow_register_threads();
}

void ar_memory_shutdown(void) {}

void *ar_alloc(size_t size) {
  void *ptr = GC_MALLOC(size);
  return ptr ? ptr : oom("allocate", size);
}

void *ar_realloc(void *old, size_t size) {
  void *ptr = GC_REALLOC(old, size);
  return ptr ? ptr : oom("reallocate", size);
}

void *ar_alloc_atomic(size_t size) {
  void *ptr = GC_MALLOC_ATOMIC(size);
  return ptr ? ptr : oom("allocate", size);
}

void *ar_alloc_uncollectable(size_t size) {
  void *ptr = GC_MALLOC_UNCOLLECTABLE(size);
  return ptr ? ptr : oom("allocate", size);
}

void ar_free(void *ptr) { GC_FREE(ptr); }

void ar_finalizer(void *obj, ar_finalizer_fn fn, void *client_data) {
  GC_register_finalizer_no_order(obj, fn, client_data, NULL, NULL);
}

int ar_register_thread(void) {
  struct GC_stack_base sb;
  GC_get_stack_base(&sb);
  return GC_register_my_thread(&sb);
}

int ar_unregister_thread(void) { return GC_unregister_my_thread(); }

void ar_gc_collect(void) { GC_gcollect(); }

#endif // ARGON_GC_CUSTOM
