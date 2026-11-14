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

#define _GNU_SOURCE // RTLD_NEXT
#include "../memory.h"
#include "gc.h"
#include <dlfcn.h>
#include <pthread.h>
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
void ar_free_owned(void *ptr) { free(ptr); }

void ar_finalizer(void *obj, ar_finalizer_fn fn, void *client_data) {
  // Nothing registers finalizers today; see README.md, stage 7.
  (void)obj, (void)fn, (void)client_data;
}

// The collector is single-threaded until stage 6. Once a second thread
// exists, allocation takes a lock and safe points stop freeing (see gc.c).
//
// That switch has to flip *before* the new thread runs, not when it gets
// round to calling ar_register_thread: a native that starts a thread often
// hands it objects that nothing counts (the threading module passes a fresh
// bound method), and a safe point on this thread in between would free them.
// So, like Boehm's GC_pthread_create, we wrap pthread_create itself. The
// executable exports this one symbol (see the Makefile), so calls from native
// modules land here too.
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start)(void *), void *arg) {
  static int (*real_pthread_create)(pthread_t *, const pthread_attr_t *,
                                    void *(*)(void *), void *);
  if (!real_pthread_create)
    *(void **)&real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
  if (!real_pthread_create) {
    fprintf(stderr, "panic: gc: can't find the real pthread_create\n");
    abort();
  }
  gc_enable_thread_safety();
  return real_pthread_create(thread, attr, start, arg);
}

int ar_register_thread(void) {
  gc_enable_thread_safety(); // already on (pthread_create), but be safe
  return 0;
}
int ar_unregister_thread(void) { return 0; }

void ar_gc_collect(void) { gc_collect_now(); }

void ar_memory_init(void) { gc_init(); }
void ar_memory_shutdown(void) { gc_shutdown(); }

#endif // ARGON_GC_CUSTOM
