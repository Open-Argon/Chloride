/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// The page allocator behind the custom GC. It knows nothing about objects,
// refcounts or types: it hands out zeroed cells, takes them back, and can
// enumerate every cell that is currently allocated. See README.md.

#ifndef ARGON_GC_HEAP_H
#define ARGON_GC_HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void heap_init(void);

// Returns zeroed memory, aligned to 16 bytes. Never returns NULL.
void *heap_alloc(size_t size);

// `ptr` must be exactly a pointer returned by heap_alloc.
void heap_free(void *ptr);

// If `ptr` points anywhere inside an allocated cell, returns the start of that
// cell; otherwise NULL. Used by debug checks ("is this a live object?").
void *heap_cell_of(const void *ptr);

// Usable size of the cell starting at `cell` (>= the size requested).
size_t heap_cell_size(const void *cell);

// Calls fn(cell, ctx) for every allocated cell. fn may free the cell it is
// given, but must not allocate or free any other cell.
void heap_for_each(void (*fn)(void *cell, void *ctx), void *ctx);

// Return completely empty pages to the OS (or a small spare pool). Freeing
// cells never does this by itself; call it after a batch of frees.
void heap_trim(void);

typedef struct {
  size_t mapped_bytes;    // memory taken from the OS for pages
  size_t allocated_bytes; // sum of cell sizes currently handed out
  size_t allocated_cells;
} HeapStats;

HeapStats heap_stats(void);

#endif // ARGON_GC_HEAP_H
