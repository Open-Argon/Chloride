/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// memory.h implementation for `make GC=custom`.
//
// Managed objects (ArgonObject, Stack) come from gc_new (gc.c, heap.c).
// Everything else that memory.h hands out - buffers owned by objects or by
// the VM - is plain malloc memory: owners release it with ar_free_owned
// (finalizers, the VM when frames end). Anything allocated with ar_alloc and
// never handed to ar_free_owned simply isn't freed.

#ifdef ARGON_GC_CUSTOM

#include "../memory.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <pthread.h> // pthread_cleanup_push
#endif

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

// ---------------------------------------------------------------------------
// Threads
//
// The collector has to know about a thread *before* it runs, not when it gets
// round to calling register_thread: a native that starts a thread often hands
// it objects nothing counts (the threading module passes a fresh bound
// method), and a collection in between would free them. And it has to know
// when the thread is gone. So whoever creates a thread passes its start
// routine through ar_prepare_thread first, and starts the thread with what
// comes back: a trampoline that adopts the GC record made here (in the
// parent), runs the real start routine, and finishes the record afterwards.
// This works the same with pthread_create and with Windows' CreateThread.
// ---------------------------------------------------------------------------

typedef struct {
  void *(*start)(void *);
  void *arg;
  void *gc_thread; // its record, made by the parent (gc_thread_started)
} ThreadStart;

#ifndef _WIN32
static void thread_done(void *gc_thread) { gc_thread_finished(gc_thread); }
#endif

static void *thread_trampoline(void *p) {
  ThreadStart ts = *(ThreadStart *)p;
  free(p);
  gc_thread_adopt(ts.gc_thread);
  void *result;
#ifdef _WIN32
  // (A thread that ends with ExitThread instead of returning never finishes
  // its record; the GC then just stays a little more conservative.)
  result = ts.start(ts.arg);
  gc_thread_finished(ts.gc_thread);
#else
  // Also runs if the thread calls pthread_exit or is cancelled.
  pthread_cleanup_push(thread_done, ts.gc_thread);
  result = ts.start(ts.arg);
  pthread_cleanup_pop(1);
#endif
  return result;
}

void ar_prepare_thread(void *(**start)(void *), void **arg) {
  ThreadStart *ts = check(malloc(sizeof *ts), sizeof *ts);
  *ts = (ThreadStart){*start, *arg, gc_thread_started()};
  *start = thread_trampoline;
  *arg = ts;
}

void ar_cancel_thread(void *arg) {
  ThreadStart *ts = arg;
  gc_thread_finished(ts->gc_thread);
  free(ts);
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
