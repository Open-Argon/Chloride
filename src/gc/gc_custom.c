/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// memory.h implementation for `make GC=custom`.
//
// TRANSITIONAL. Untyped allocations (ar_alloc & friends) still come from
// calloc and are never freed, so a GC=custom build currently leaks everything.
// That gives us a working baseline: as each allocation site is converted to a
// managed type with a refcount (README.md, stages 3-4), it moves off this file
// and onto the real collector. When nothing calls ar_alloc any more, this file
// shrinks to init/shutdown.

#ifdef ARGON_GC_CUSTOM

#include "../memory.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>

static void *check(void *ptr, size_t size) {
  if (!ptr) {
    fprintf(stderr, "panic: unable to allocate memory: %zu\n", size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *ar_alloc(size_t size) { return check(calloc(1, size ? size : 1), size); }
void *ar_alloc_atomic(size_t size) { return ar_alloc(size); }
void *ar_alloc_uncollectable(size_t size) { return ar_alloc(size); }

void *ar_realloc(void *old, size_t size) {
  return check(realloc(old, size ? size : 1), size);
}

void ar_free(void *ptr) { free(ptr); }

void ar_finalizer(void *obj, ar_finalizer_fn fn, void *client_data) {
  // Nothing registers finalizers today; see README.md, stage 7.
  (void)obj, (void)fn, (void)client_data;
}

// The collector is single-threaded until stage 6; for now a second thread
// only switches allocation over to a lock.
int ar_register_thread(void) {
  gc_enable_thread_safety();
  return 0;
}
int ar_unregister_thread(void) { return 0; }

void ar_gc_collect(void) { gc_collect_now(); }

void ar_memory_init(void) { gc_init(); }
void ar_memory_shutdown(void) { gc_shutdown(); }

#endif // ARGON_GC_CUSTOM
