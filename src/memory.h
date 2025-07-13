/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARGON_MEMORY_H
#define ARGON_MEMORY_H

#include <stddef.h> // for size_t
#include <gc/gc.h>

// GC-managed allocations

void ar_finalizer(void *obj, GC_finalization_proc fn, void *client_data,
                  GC_finalization_proc *old_fn, void **old_client_data);
void *ar_alloc(size_t size);
void *ar_alloc_atomic(size_t size);
char *ar_strdup(const char *str);

// Memory init/shutdown
void ar_memory_init();

void *checked_malloc(size_t size);

#endif // ARGON_MEMORY_H