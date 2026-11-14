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
#include <errno.h>
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

// Threads. Freeing has to switch off *before* a new thread runs, not when it
// gets round to calling ar_register_thread: a native that starts a thread
// often hands it objects nothing counts (the threading module passes a fresh
// bound method), and a safe point on this thread in between would free them.
// It has to switch back on only once the thread can't touch objects any
// more. So, like Boehm's GC_pthread_create, we wrap pthread_create itself:
// the executable exports this one symbol (see the Makefile), so calls from
// native modules land here too, and the new thread runs a trampoline that
// reports when its start routine is done.

typedef struct {
  void *(*start)(void *);
  void *arg;
  void *gc_thread; // its record, made by the parent (gc_thread_started)
} ThreadStart;

static void thread_done(void *gc_thread) { gc_thread_finished(gc_thread); }

static void *thread_trampoline(void *p) {
  ThreadStart ts = *(ThreadStart *)p;
  free(p);
  gc_thread_adopt(ts.gc_thread);
  void *result;
  // Also runs if the thread calls pthread_exit or is cancelled.
  pthread_cleanup_push(thread_done, ts.gc_thread);
  result = ts.start(ts.arg);
  pthread_cleanup_pop(1);
  return result;
}

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
  ThreadStart *ts = malloc(sizeof *ts);
  if (!ts)
    return EAGAIN;
  *ts = (ThreadStart){start, arg, gc_thread_started()};
  int result = real_pthread_create(thread, attr, thread_trampoline, ts);
  if (result != 0) { // no thread after all
    gc_thread_finished(ts->gc_thread);
    free(ts);
  }
  return result;
}

// The native API's register/unregister_thread: from here on (until
// unregister) this thread may touch objects, and so must stop for
// collections at safe points.
int ar_register_thread(void) {
  gc_thread_attach();
  return 0;
}
int ar_unregister_thread(void) {
  gc_thread_detach();
  return 0;
}

void ar_gc_collect(void) { gc_collect_now(); }

void ar_memory_init(void) { gc_init(); }
void ar_memory_shutdown(void) { gc_shutdown(); }

#endif // ARGON_GC_CUSTOM
