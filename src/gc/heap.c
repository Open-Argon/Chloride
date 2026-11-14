/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Page allocator for the custom GC (see heap.h and README.md).
//
// Layout
//   * Memory comes from the OS in 64 KiB pages, aligned to 64 KiB.
//   * A *small* page holds cells of one size class (16 B .. 16 KiB). Free
//     cells are kept on a per-page free list threaded through their first word.
//   * A *large* object (> 16 KiB) gets its own run of pages.
//   * Each page has an "allocated" bitmap, so we can enumerate live cells.
//   * A two-level page table maps any address to its page, so we can answer
//     "which cell, if any, does this pointer point into?" quickly.

#ifdef ARGON_GC_CUSTOM

#include "heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#define PAGE_SHIFT 16
#define PAGE_SIZE ((uintptr_t)1 << PAGE_SHIFT) // 64 KiB
#define PAGE_MASK (~(PAGE_SIZE - 1))
#ifdef _WIN32
// VirtualFree can only release a whole allocation, so map pages one by one.
#define CHUNK_PAGES 1
#else
#define CHUNK_PAGES 16 // pages mapped from the OS at a time
#endif
#define MAX_SPARE_PAGES 64 // empty pages kept for reuse before unmapping
#define GRANULE 16

// Size classes: a request is rounded up to the next one. Spacing grows with
// size so that rounding never wastes more than ~25% of a cell.
static const uint32_t class_sizes[] = {
    16,   32,   48,   64,   80,   96,   112,  128,  160,   192,   224,  256,
    320,  384,  448,  512,  640,  768,  896,  1024, 1280,  1536,  1792, 2048,
    2560, 3072, 3584, 4096, 5120, 6144, 7168, 8192, 10240, 12288, 14336, 16384};
#define NUM_CLASSES (sizeof(class_sizes) / sizeof(class_sizes[0]))
#define MAX_SMALL 16384
#define LARGE_CLASS 0xff

static uint8_t size_to_class[MAX_SMALL / GRANULE + 1];

// ---------------------------------------------------------------------------
// Pages
// ---------------------------------------------------------------------------

typedef struct Page {
  uintptr_t start;   // first byte of the page (or of the large object)
  size_t cell_size;  // bytes per cell (large: the whole page-rounded object)
  size_t ncells;     // cells in this page (1 for large objects)
  size_t npages;     // 64 KiB pages covered (1 for small pages)
  size_t live;       // currently allocated cells
  uint8_t cls;       // index into class_sizes, or LARGE_CLASS
  bool in_partial;   // on its size class's "has free cells" list
  void *free_list;   // free cells, linked through their first word
  struct Page *prev_all, *next_all; // every page of this class (or large)
  struct Page *next_partial;        // pages with at least one free cell
  uint64_t alloc_bits[];            // 1 = cell is allocated
} Page;

typedef struct {
  Page *all;
  Page *partial;
} SizeClass;

static SizeClass classes[NUM_CLASSES];
static Page *large_objects;
static HeapStats stats;

static inline bool bit_get(const uint64_t *b, size_t i) {
  return (b[i >> 6] >> (i & 63)) & 1;
}
static inline void bit_set(uint64_t *b, size_t i) {
  b[i >> 6] |= (uint64_t)1 << (i & 63);
}
static inline void bit_clear(uint64_t *b, size_t i) {
  b[i >> 6] &= ~((uint64_t)1 << (i & 63));
}
static inline size_t bitmap_words(size_t n) { return (n + 63) / 64; }

static void oom(size_t size) {
  fprintf(stderr, "panic: unable to allocate memory: %zu\n", size);
  exit(EXIT_FAILURE);
}

// ---------------------------------------------------------------------------
// Page table: a two-level radix tree over the page number (address >> 16) of
// a 48-bit address space. Lookups are two array loads.
// ---------------------------------------------------------------------------

#define ADDR_BITS 48
#define LEAF_BITS 16
#define LEAF_SIZE ((size_t)1 << LEAF_BITS)
#define ROOT_SIZE ((size_t)1 << (ADDR_BITS - PAGE_SHIFT - LEAF_BITS))

static Page ***page_table;

// ---------------------------------------------------------------------------
// The OS layer: everything heap.c needs from the operating system.
//   os_map(len)          zeroed, readable+writable memory
//   os_map_aligned(n)    n pages, aligned to PAGE_SIZE (counted in stats)
//   os_unmap(start, len) give back something from os_map_aligned
//   os_discard(start)    keep a spare page mapped but drop its contents
// ---------------------------------------------------------------------------

#ifdef _WIN32

static void *os_map(size_t len) {
  void *m = VirtualAlloc(NULL, len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!m)
    oom(len);
  return m;
}

// Windows hands out memory in units of its allocation granularity, which is
// 64 KiB (= PAGE_SIZE) everywhere in practice, so this is normally aligned
// already. If not, reserve a bigger range to find an aligned address inside
// it, release it, and claim exactly that address (retrying if another thread
// took it in between).
static void *os_map_aligned_raw(size_t len) {
  void *m = os_map(len);
  if (((uintptr_t)m & (PAGE_SIZE - 1)) == 0)
    return m;
  VirtualFree(m, 0, MEM_RELEASE);
  for (;;) {
    void *range = VirtualAlloc(NULL, len + PAGE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
    if (!range)
      oom(len);
    uintptr_t aligned = ((uintptr_t)range + PAGE_SIZE - 1) & PAGE_MASK;
    VirtualFree(range, 0, MEM_RELEASE);
    m = VirtualAlloc((void *)aligned, len, MEM_RESERVE | MEM_COMMIT,
                     PAGE_READWRITE);
    if (m)
      return m;
  }
}

static void os_release(uintptr_t start, size_t len) {
  (void)len; // VirtualFree releases the whole allocation starting there
  VirtualFree((void *)start, 0, MEM_RELEASE);
}

static void os_discard(uintptr_t start) {
  // The contents can be thrown away; the page stays mapped. (It isn't
  // zeroed, which is fine: the allocator zeroes every cell it hands out.)
  VirtualAlloc((void *)start, PAGE_SIZE, MEM_RESET, PAGE_READWRITE);
}

#else // POSIX

static void *os_map(size_t len) {
  void *m = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                 -1, 0);
  if (m == MAP_FAILED)
    oom(len);
  return m;
}

// mmap only guarantees OS-page alignment (4 or 16 KiB), so over-map by one
// of our pages and trim.
static void *os_map_aligned_raw(size_t len) {
  uint8_t *raw = os_map(len + PAGE_SIZE);
  uintptr_t start = ((uintptr_t)raw + PAGE_SIZE - 1) & PAGE_MASK;
  size_t head = start - (uintptr_t)raw;
  if (head)
    munmap(raw, head);
  if (PAGE_SIZE - head)
    munmap((void *)(start + len), PAGE_SIZE - head);
  return (void *)start;
}

static void os_release(uintptr_t start, size_t len) {
  munmap((void *)start, len);
}

static void os_discard(uintptr_t start) {
  // Drop the physical memory; the page reads as zero when next touched.
  madvise((void *)start, PAGE_SIZE, MADV_DONTNEED);
}

#endif

static inline Page *page_lookup(uintptr_t addr) {
  if (addr >> ADDR_BITS)
    return NULL;
  uintptr_t pn = addr >> PAGE_SHIFT;
  Page **leaf = page_table[pn >> LEAF_BITS];
  return leaf ? leaf[pn & (LEAF_SIZE - 1)] : NULL;
}

static void page_table_set(uintptr_t start, size_t npages, Page *p) {
  if ((start + npages * PAGE_SIZE - 1) >> ADDR_BITS) {
    // Every current 64-bit OS gives user programs at most 47-48 bit
    // addresses; the page table would need another level for more.
    fprintf(stderr, "panic: gc: address %p is beyond %d bits\n",
            (void *)start, ADDR_BITS);
    abort();
  }
  for (size_t i = 0; i < npages; i++) {
    uintptr_t pn = (start >> PAGE_SHIFT) + i;
    Page ***leaf = &page_table[pn >> LEAF_BITS];
    if (!*leaf)
      *leaf = os_map(LEAF_SIZE * sizeof(Page *));
    (*leaf)[pn & (LEAF_SIZE - 1)] = p;
  }
}

// ---------------------------------------------------------------------------
// Getting pages from the OS
// ---------------------------------------------------------------------------

static uintptr_t spare_pages[MAX_SPARE_PAGES];
static size_t spare_count;

static uintptr_t os_map_aligned(size_t npages) {
  size_t len = npages * PAGE_SIZE;
  uintptr_t start = (uintptr_t)os_map_aligned_raw(len);
  stats.mapped_bytes += len;
  return start;
}

static void os_unmap(uintptr_t start, size_t len) {
  os_release(start, len);
  stats.mapped_bytes -= len;
}

static uintptr_t take_page(void) {
  if (spare_count == 0) {
    uintptr_t chunk = os_map_aligned(CHUNK_PAGES);
    for (size_t i = CHUNK_PAGES; i-- > 0;)
      spare_pages[spare_count++] = chunk + i * PAGE_SIZE;
  }
  return spare_pages[--spare_count];
}

static void give_back_page(uintptr_t start) {
  if (spare_count < MAX_SPARE_PAGES) {
    os_discard(start);
    spare_pages[spare_count++] = start;
  } else {
    os_unmap(start, PAGE_SIZE);
  }
}

// ---------------------------------------------------------------------------
// Page lists
// ---------------------------------------------------------------------------

static Page **all_list(Page *p) {
  return p->cls == LARGE_CLASS ? &large_objects : &classes[p->cls].all;
}

static void link_all(Page *p) {
  Page **head = all_list(p);
  p->prev_all = NULL;
  p->next_all = *head;
  if (*head)
    (*head)->prev_all = p;
  *head = p;
}

static void unlink_all(Page *p) {
  if (p->prev_all)
    p->prev_all->next_all = p->next_all;
  else
    *all_list(p) = p->next_all;
  if (p->next_all)
    p->next_all->prev_all = p->prev_all;
}

static void push_partial(Page *p) {
  SizeClass *sc = &classes[p->cls];
  p->next_partial = sc->partial;
  sc->partial = p;
  p->in_partial = true;
}

// Give completely empty small pages back (to the spare pool or the OS) and
// rebuild every partial list.
void heap_trim(void) {
  for (size_t cls = 0; cls < NUM_CLASSES; cls++) {
    SizeClass *sc = &classes[cls];
    sc->partial = NULL;
    Page *p = sc->all;
    while (p) {
      Page *next = p->next_all;
      p->in_partial = false;
      if (p->live == 0) {
        unlink_all(p);
        page_table_set(p->start, 1, NULL);
        give_back_page(p->start);
        free(p);
      } else if (p->free_list) {
        push_partial(p);
      }
      p = next;
    }
  }
}

// ---------------------------------------------------------------------------
// Allocation
// ---------------------------------------------------------------------------

static Page *new_small_page(int cls) {
  size_t cell = class_sizes[cls];
  size_t ncells = PAGE_SIZE / cell;
  Page *p = calloc(1, sizeof(Page) + bitmap_words(ncells) * sizeof(uint64_t));
  if (!p)
    oom(sizeof(Page));
  p->start = take_page();
  p->cell_size = cell;
  p->ncells = ncells;
  p->npages = 1;
  p->cls = cls;
  // Thread every cell onto the free list, lowest address first.
  void **link = &p->free_list;
  for (size_t i = 0; i < ncells; i++) {
    void *c = (void *)(p->start + i * cell);
    *link = c;
    link = (void **)c;
  }
  *link = NULL;
  page_table_set(p->start, 1, p);
  link_all(p);
  push_partial(p);
  return p;
}

static void *alloc_small(int cls) {
  SizeClass *sc = &classes[cls];
  Page *p = sc->partial;
  if (!p)
    p = new_small_page(cls);
  void *cell = p->free_list;
  p->free_list = *(void **)cell;
  bit_set(p->alloc_bits, ((uintptr_t)cell - p->start) / p->cell_size);
  p->live++;
  if (!p->free_list) { // now full: off the partial list (it's the head)
    sc->partial = p->next_partial;
    p->in_partial = false;
  }
  memset(cell, 0, p->cell_size);
  return cell;
}

static void *alloc_large(size_t size) {
  size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  Page *p = calloc(1, sizeof(Page) + sizeof(uint64_t));
  if (!p)
    oom(sizeof(Page));
  p->start = os_map_aligned(npages); // fresh OS memory is already zero
  p->cell_size = npages * PAGE_SIZE;
  p->ncells = 1;
  p->npages = npages;
  p->cls = LARGE_CLASS;
  p->live = 1;
  bit_set(p->alloc_bits, 0);
  page_table_set(p->start, npages, p);
  link_all(p);
  return (void *)p->start;
}

void *heap_alloc(size_t size) {
  if (size == 0)
    size = 1;
  void *cell = size <= MAX_SMALL
                   ? alloc_small(size_to_class[(size + GRANULE - 1) / GRANULE])
                   : alloc_large(size);
  stats.allocated_bytes += heap_cell_size(cell);
  stats.allocated_cells++;
  return cell;
}

void heap_free(void *ptr) {
  Page *p = page_lookup((uintptr_t)ptr);
  size_t idx = p && p->cls != LARGE_CLASS
                   ? ((uintptr_t)ptr - p->start) / p->cell_size
                   : 0;
  if (!p || (uintptr_t)ptr != p->start + idx * p->cell_size ||
      !bit_get(p->alloc_bits, idx)) {
    fprintf(stderr, "panic: gc: heap_free of invalid pointer %p\n", ptr);
    abort();
  }
  stats.allocated_bytes -= p->cell_size;
  stats.allocated_cells--;
  bit_clear(p->alloc_bits, idx);
  p->live--;

  if (p->cls == LARGE_CLASS) {
    // Large pages are unlinked straight away; heap_for_each reads next_all
    // before calling fn, so this is safe even while iterating.
    unlink_all(p);
    page_table_set(p->start, p->npages, NULL);
    os_unmap(p->start, p->cell_size);
    free(p);
    return;
  }

  // Small cell: back on its page's free list. Empty pages stay put until
  // heap_trim, so freeing never invalidates a heap_for_each in progress.
  *(void **)ptr = p->free_list;
  p->free_list = ptr;
  if (!p->in_partial)
    push_partial(p);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

void *heap_cell_of(const void *ptr) {
  Page *p = page_lookup((uintptr_t)ptr);
  if (!p)
    return NULL;
  size_t idx = 0;
  if (p->cls != LARGE_CLASS) {
    idx = ((uintptr_t)ptr - p->start) / p->cell_size;
    if (idx >= p->ncells) // the unused tail at the end of a page
      return NULL;
  }
  if (!bit_get(p->alloc_bits, idx))
    return NULL;
  return (void *)(p->start + idx * p->cell_size);
}

size_t heap_cell_size(const void *cell) {
  Page *p = page_lookup((uintptr_t)cell);
  return p ? p->cell_size : 0;
}

void heap_for_each(void (*fn)(void *cell, void *ctx), void *ctx) {
  for (size_t cls = 0; cls < NUM_CLASSES; cls++) {
    for (Page *p = classes[cls].all; p; p = p->next_all) {
      for (size_t w = 0; w < bitmap_words(p->ncells); w++) {
        uint64_t bits = p->alloc_bits[w];
        while (bits) {
          size_t i = w * 64 + __builtin_ctzll(bits);
          bits &= bits - 1;
          fn((void *)(p->start + i * p->cell_size), ctx);
        }
      }
    }
  }
  for (Page *p = large_objects, *next; p; p = next) {
    next = p->next_all;
    fn((void *)p->start, ctx);
  }
}

HeapStats heap_stats(void) { return stats; }

void heap_init(void) {
  for (size_t g = 0, cls = 0; g <= MAX_SMALL / GRANULE; g++) {
    while (class_sizes[cls] < g * GRANULE)
      cls++;
    size_to_class[g] = (uint8_t)cls;
  }
  page_table = os_map(ROOT_SIZE * sizeof(Page **));
}

#endif // ARGON_GC_CUSTOM
