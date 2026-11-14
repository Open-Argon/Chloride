/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Boehm-Demers-Weiser GC backend. The default unless built with GC=custom.

#ifndef ARGON_GC_CUSTOM

#define GC_THREADS
#include "../memory.h"
#include "gc.h"
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
void ar_free_owned(void *ptr) { (void)ptr; }

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

// ---------------------------------------------------------------------------
// gc.h on top of Boehm: managed objects are ordinary Boehm allocations and
// every counting / collection operation is a no-op, because Boehm finds
// garbage by itself. Note that Boehm never calls GCKind.finalize, so memory a
// managed object "owns" must come from Boehm too in this build.
// ---------------------------------------------------------------------------

static size_t kind_count;

uint8_t gc_register_kind(const GCKind *kind) {
  (void)kind;
  return (uint8_t)kind_count++;
}

void *gc_new(uint8_t kind, size_t size) {
  GCHeader *h = ar_alloc(size);
  h->kind = kind;
  h->flags = GC_MORTAL;
  return h;
}

size_t gc_object_size(const void *obj) { return GC_size(obj); }
void gc_immortalize(void *obj) { (void)obj; }
void gc_immortalize_all(void) {}
void gc_register_static(void *obj) { (void)obj; }
gc_epoch gc_enter(void) { return 0; }
void gc_leave(gc_epoch previous) { (void)previous; }
void gc_collect_now(void) { GC_gcollect(); }
void gc_add_counted_roots(gc_root_tracer tracer) { (void)tracer; }
void gc_add_uncounted_roots(gc_root_tracer tracer) { (void)tracer; }
GCVerifyResult gc_verify(void) { return (GCVerifyResult){0}; }
GCStats gc_stats(void) { return (GCStats){0}; }
void gc_init(void) {}
void gc_thread_started(void) {}
void gc_thread_finished(void) {}
void gc_shutdown(void) {}

#endif // ARGON_GC_CUSTOM
