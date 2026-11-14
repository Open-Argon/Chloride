/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARGON_MEMORY_H
#define ARGON_MEMORY_H

// This header is the *only* interface the rest of the interpreter uses to talk
// to the garbage collector. Nothing outside src/gc/ should include <gc/gc.h>
// or call GC_* directly, so the backend can be swapped at build time:
//
//   make            -> Boehm backend   (src/gc/gc_boehm.c)
//   make GC=custom  -> custom backend  (src/gc/gc_custom.c)

#include <stdbool.h>
#include <stddef.h> // for size_t

typedef void (*ar_finalizer_fn)(void *obj, void *client_data);

// --- collected heap -------------------------------------------------------
// Memory that may contain pointers to other collected memory.
void *ar_alloc(size_t size);
void *ar_realloc(void *old, size_t size);
// Memory that will never contain pointers (strings, bytecode, GMP limbs).
// The collector does not scan it, so never store a GC pointer in it.
void *ar_alloc_atomic(size_t size);
char *ar_strdup(const char *str);

void ar_finalizer(void *obj, ar_finalizer_fn fn, void *client_data);

// Free a buffer that exactly one owner (a managed object, or the VM) holds,
// from ar_alloc / ar_alloc_atomic / ar_realloc. Called by GC finalizers and
// when the VM drops frames. With Boehm it does nothing: Boehm reclaims the
// memory by itself, and freeing early could only add risk there.
void ar_free_owned(void *ptr);

// --- uncollectable heap ---------------------------------------------------
// Never freed by the collector, but scanned for pointers (i.e. acts as a root).
// Must be released with ar_free.
void *ar_alloc_uncollectable(size_t size);
void ar_free(void *ptr);

// --- threads ----------------------------------------------------------------
// Before starting a thread (with pthread_create, CreateThread, ...), pass its
// start routine and argument through ar_prepare_thread, and start the thread
// with what it gives back. That tells the collector about the thread before
// it runs. If creating the thread then fails, call ar_cancel_thread with the
// argument ar_prepare_thread gave you.
void ar_prepare_thread(void *(**start)(void *), void **arg);
void ar_cancel_thread(void *arg);

// Any thread other than the main thread must register before touching the
// heap, and unregister before it exits.
int ar_register_thread(void);
int ar_unregister_thread(void);

// --- control ----------------------------------------------------------------
void ar_gc_collect(void);

void ar_memory_init(void);
void ar_memory_shutdown(void);

// --- plain malloc (not collected, not scanned) ------------------------------
// Never store a GC pointer in memory from these: the collector can't see it.
void *checked_malloc(size_t size);
void *checked_realloc(void *ptr, size_t size);

#endif // ARGON_MEMORY_H
