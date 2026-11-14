/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Standalone randomized test for the page allocator. Run with `make gc-test`.
// Keeps its own list of live allocations and checks the heap agrees with it.

#include "../heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

typedef struct {
  unsigned char *ptr;
  size_t size;
  unsigned char fill;
} Alloc;

#define MAX_LIVE 20000
static Alloc live[MAX_LIVE];
static size_t nlive;

static size_t random_size(void) {
  int r = rand() % 100;
  if (r < 70)
    return 1 + rand() % 128; // mostly small, like real objects
  if (r < 97)
    return 1 + rand() % 16384;
  return 16385 + rand() % 300000; // occasionally large
}

static void count_cell(void *cell, void *ctx) {
  (void)cell;
  (*(size_t *)ctx)++;
}

static void check_all(void) {
  size_t counted = 0;
  heap_for_each(count_cell, &counted);
  CHECK(counted == nlive);
  CHECK(heap_stats().allocated_cells == nlive);
  for (size_t i = 0; i < nlive; i++) {
    Alloc *a = &live[i];
    CHECK(heap_cell_of(a->ptr) == a->ptr);
    CHECK(heap_cell_of(a->ptr + a->size - 1) == a->ptr); // interior pointer
    CHECK(heap_cell_size(a->ptr) >= a->size);
    for (size_t j = 0; j < a->size; j++)
      CHECK(a->ptr[j] == a->fill); // nobody else wrote into our cell
  }
}

int main(void) {
  heap_init();
  srand(12345);

  for (int round = 0; round < 200000; round++) {
    if (nlive < MAX_LIVE && (nlive == 0 || rand() % 100 < 55)) {
      Alloc a = {.size = random_size(), .fill = (unsigned char)(1 + rand() % 255)};
      a.ptr = heap_alloc(a.size);
      CHECK(((uintptr_t)a.ptr & 15) == 0);
      for (size_t j = 0; j < a.size; j++)
        CHECK(a.ptr[j] == 0); // memory must come back zeroed
      memset(a.ptr, a.fill, a.size);
      live[nlive++] = a;
    } else {
      size_t i = rand() % nlive;
      unsigned char *p = live[i].ptr;
      heap_free(p);
      CHECK(heap_cell_of(p) == NULL); // freed cells are not "live"
      live[i] = live[--nlive];
    }
    if (round % 20000 == 0) {
      check_all();
      heap_trim();
      check_all();
    }
  }

  // Free everything; trimming should hand (almost) all memory back.
  while (nlive)
    heap_free(live[--nlive].ptr);
  heap_trim();
  check_all();
  HeapStats s = heap_stats();
  CHECK(s.allocated_bytes == 0);
  printf("heap_test: ok (%zu KiB still mapped as spare pages)\n",
         s.mapped_bytes / 1024);
  return 0;
}
