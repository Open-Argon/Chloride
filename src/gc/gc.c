/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Reference counting, the zero count table, epochs, safe points and the
// trial-deletion cycle collector. See gc.h for the API and README.md for the
// ideas. The verifier lives in verify.c.

#ifdef ARGON_GC_CUSTOM

#include "gc_internal.h"
#include "heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Process the ZCT once it holds this many entries (more than the ones we had
// to keep last time). Small = prompt freeing, large = less overhead.
#define ZCT_MIN 1024
// Run the cycle collector after this many allocations, or after as many
// allocations as there are live objects, whichever is larger. Each cycle
// collection walks the whole heap, so scaling with heap size keeps the
// average cost per allocation constant.
#define CYCLE_MIN 10000

GCConfig gc_config;
const GCKind *gc_kinds[256];
size_t gc_kind_count;
GCHeader **gc_statics;
size_t gc_static_count;
gc_root_tracer *gc_root_tracers;
size_t gc_root_tracer_count;

bool gc_work_due;

static uint64_t birth_counter;  // birth number of the next allocation
static gc_epoch current_epoch;  // objects born before this can't be freed now
static bool in_collection;      // guards against re-entering a safe point
static bool stress_pending;     // ARGON_GC_STRESS asked for a full safe point
static size_t stress_counter;

// The zero count table: objects whose refcount is (or was, when listed) 0.
static GCHeader **zct;
static size_t zct_len, zct_cap;
static size_t zct_limit = ZCT_MIN;

static size_t allocs_since_cycle;
static size_t cycle_threshold = CYCLE_MIN;

static GCStats stats;
static double last_cycle_pause_ms;

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static void *xrealloc(void *p, size_t size) {
  p = realloc(p, size);
  if (!p) {
    fprintf(stderr, "panic: gc: out of memory (%zu bytes)\n", size);
    exit(EXIT_FAILURE);
  }
  return p;
}

void gc_vec_push(GCVec *v, GCHeader *h) {
  if (v->len == v->cap) {
    v->cap = v->cap ? v->cap * 2 : 1024;
    v->items = xrealloc(v->items, v->cap * sizeof(GCHeader *));
  }
  v->items[v->len++] = h;
}

void gc_vec_free(GCVec *v) {
  free(v->items);
  *v = (GCVec){0};
}

void gc_describe(GCHeader *h, char *buf, size_t len) {
  const GCKind *k = gc_kind_of(h);
  int n = snprintf(buf, len, "%s@%p", k ? k->name : "?", (void *)h);
  if (k && k->describe && n > 0 && (size_t)n + 4 < len) {
    buf[n++] = ' ';
    buf[n++] = '(';
    k->describe(h, buf + n, len - n - 1);
    size_t end = strlen(buf);
    if (end + 1 < len) {
      buf[end] = ')';
      buf[end + 1] = '\0';
    }
  }
}

// ---------------------------------------------------------------------------
// Kinds, allocation, counting
// ---------------------------------------------------------------------------

uint8_t gc_register_kind(const GCKind *kind) {
  if (gc_kind_count == 256) {
    fprintf(stderr, "panic: gc: too many kinds\n");
    abort();
  }
  gc_kinds[gc_kind_count] = kind;
  return (uint8_t)gc_kind_count++;
}

static void zct_push(GCHeader *h) {
  if (zct_len == zct_cap) {
    zct_cap = zct_cap ? zct_cap * 2 : 4096;
    zct = xrealloc(zct, zct_cap * sizeof(GCHeader *));
  }
  zct[zct_len++] = h;
  h->flags |= GC_IN_ZCT;
  if (zct_len >= zct_limit)
    gc_work_due = true;
}

void *gc_new(uint8_t kind, size_t size) {
  if (kind >= gc_kind_count) {
    fprintf(stderr, "panic: gc: gc_new with unregistered kind %u\n", kind);
    abort();
  }
  GCHeader *h = heap_alloc(size); // zeroed: refcount 0, gc_refs 0
  h->kind = kind;
  h->flags = GC_MORTAL;
  h->birth = birth_counter++;
  stats.allocated++;
  stats.live++;

  // A new object has no counted references yet. If nothing stores it before
  // the next safe point, it's garbage, so it starts out in the ZCT.
  zct_push(h);

  if (++allocs_since_cycle >= cycle_threshold)
    gc_work_due = true;
  if (gc_config.stress_every && ++stress_counter >= gc_config.stress_every) {
    stress_counter = 0;
    stress_pending = true;
    gc_work_due = true;
  }
  return h;
}

void gc_zero_count(GCHeader *h) {
  if (!(h->flags & GC_IN_ZCT))
    zct_push(h);
}

void gc_bad_decref(GCHeader *h) {
  char desc[160];
  gc_describe(h, desc, sizeof desc);
  if (h->flags & GC_ZOMBIE)
    fprintf(stderr, "gc: decref of %s, which was already freed\n", desc);
  else
    fprintf(stderr,
            "gc: decref of %s, whose refcount is already 0 (some store into a "
            "counted holder is missing its incref)\n",
            desc);
  if (!gc_config.nofree)
    abort(); // memory is about to be corrupted; stop while it's debuggable
}

void gc_immortalize(void *obj) {
  GCHeader *h = obj;
  if (h)
    h->flags &= ~GC_MORTAL; // the ZCT drops it next time it is processed
}

static void immortalize_cell(void *cell, void *ctx) {
  (void)ctx;
  gc_immortalize(cell);
}

void gc_immortalize_all(void) { heap_for_each(immortalize_cell, NULL); }

void gc_register_static(void *obj) {
  gc_statics = xrealloc(gc_statics, (gc_static_count + 1) * sizeof *gc_statics);
  gc_statics[gc_static_count++] = obj;
}

void gc_add_root_tracer(gc_root_tracer tracer) {
  gc_root_tracers = xrealloc(gc_root_tracers, (gc_root_tracer_count + 1) *
                                                  sizeof *gc_root_tracers);
  gc_root_tracers[gc_root_tracer_count++] = tracer;
}

// ---------------------------------------------------------------------------
// Epochs
// ---------------------------------------------------------------------------

gc_epoch gc_enter(void) {
  gc_epoch previous = current_epoch;
  current_epoch = birth_counter;
  return previous;
}

void gc_leave(gc_epoch previous) { current_epoch = previous; }

// ---------------------------------------------------------------------------
// Freeing
// ---------------------------------------------------------------------------

// Drop the reference a dying object holds on `child`. Children that are
// themselves part of a garbage cycle being freed are skipped: they're going
// away regardless, and their counts no longer matter.
static void release_child(GCHeader *child, void *ctx) {
  (void)ctx;
  if (child && !(child->flags & GC_DYING))
    gc_decref(child); // may put the child in the ZCT
}

// Give the memory back (the object's references are already released).
static void destroy(GCHeader *h) {
  if (gc_config.nofree) {
    // Debug mode: keep the memory so later use of it can't crash, and mark
    // it so the verifier can report anything still pointing here.
    h->flags = (h->flags & ~GC_DYING) | GC_ZOMBIE;
    return;
  }
  const GCKind *k = gc_kind_of(h);
  if (k->finalize)
    k->finalize(h);
  heap_free(h);
  stats.live--;
}

static void free_object(GCHeader *h) {
  const GCKind *k = gc_kind_of(h);
  if (k->traverse)
    k->traverse(h, release_child, NULL);
  destroy(h);
}

// Walk the ZCT. Every entry is one of:
//   * counted again (refcount > 0), or immortal, or a zombie: just drop it
//   * dead (refcount 0) and born in the current epoch: free it
//   * dead but older than the current epoch: a suspended native might still
//     be holding it, so keep it for a shallower safe point
// Freeing an object decrefs its children, which may append them to the ZCT;
// the loop picks those up too, so whole structures go in one pass with no
// recursion.
static void process_zct(void) {
  size_t kept = 0;
  for (size_t i = 0; i < zct_len; i++) {
    GCHeader *h = zct[i];
    if (h->refcount > 0 || !(h->flags & GC_MORTAL) ||
        (h->flags & GC_ZOMBIE)) {
      h->flags &= ~GC_IN_ZCT;
    } else if (h->birth >= current_epoch) {
      h->flags &= ~GC_IN_ZCT;
      stats.freed_by_rc++;
      free_object(h);
    } else {
      zct[kept++] = h; // kept <= i, so this never overwrites unread entries
    }
  }
  zct_len = kept;
  // Kept entries are re-examined every time; let the table grow in
  // proportion so that doesn't turn quadratic.
  zct_limit = 2 * kept + ZCT_MIN;
}

// ---------------------------------------------------------------------------
// Cycle collection (trial deletion)
// ---------------------------------------------------------------------------

// Step 0: candidates are mortal, live, container objects born in the current
// epoch. Their gc_refs start as a copy of their refcount.
static void add_candidate(void *cell, void *ctx) {
  GCHeader *h = cell;
  if (!(h->flags & GC_MORTAL) || (h->flags & GC_ZOMBIE) ||
      h->birth < current_epoch || !gc_kind_of(h)->traverse)
    return;
  h->flags |= GC_CANDIDATE;
  h->gc_refs = h->refcount;
  gc_vec_push(ctx, h);
}

#define GC_REFS_BROKEN 0x80000000u

// Step 1: remove references that come from inside the candidate set.
static void subtract_internal(GCHeader *child, void *ctx) {
  (void)ctx;
  if (!child || !(child->flags & GC_CANDIDATE))
    return;
  if (child->gc_refs == 0) {
    // More references from candidates than its refcount admits: the count
    // is too low. Don't risk freeing it; treat it as externally referenced.
    char desc[160];
    gc_describe(child, desc, sizeof desc);
    fprintf(stderr, "gc: refcount of %s is too low (cycle collector)\n", desc);
    child->gc_refs = GC_REFS_BROKEN;
  } else if (child->gc_refs != GC_REFS_BROKEN) {
    child->gc_refs--;
  }
}

// Step 2: anything still referenced from outside is alive, and so is
// everything it reaches.
static void mark_reachable(GCHeader *child, void *ctx) {
  if (child && (child->flags & GC_CANDIDATE) &&
      !(child->flags & GC_REACHABLE)) {
    child->flags |= GC_REACHABLE;
    gc_vec_push(ctx, child);
  }
}

static void collect_cycles(void) {
  double t0 = now_ms();

  GCVec candidates = {0};
  heap_for_each(add_candidate, &candidates);

  for (size_t i = 0; i < candidates.len; i++) {
    GCHeader *h = candidates.items[i];
    gc_kind_of(h)->traverse(h, subtract_internal, NULL);
  }

  GCVec work = {0};
  for (size_t i = 0; i < candidates.len; i++) {
    GCHeader *h = candidates.items[i];
    if (h->gc_refs > 0 && !(h->flags & GC_REACHABLE)) {
      h->flags |= GC_REACHABLE;
      gc_vec_push(&work, h);
    }
  }
  while (work.len) {
    GCHeader *h = work.items[--work.len];
    gc_kind_of(h)->traverse(h, mark_reachable, &work);
  }
  gc_vec_free(&work);

  // Step 3: whatever is left is referenced only by other garbage.
  GCVec garbage = {0};
  for (size_t i = 0; i < candidates.len; i++) {
    GCHeader *h = candidates.items[i];
    if (!(h->flags & GC_REACHABLE)) {
      h->flags |= GC_DYING;
      gc_vec_push(&garbage, h);
    }
    h->flags &= ~(GC_CANDIDATE | GC_REACHABLE);
    h->gc_refs = 0;
  }
  gc_vec_free(&candidates);

  // Step 4: free it. First release every reference from garbage to
  // non-garbage (those objects may die too, via the ZCT), then free memory.
  // Garbage objects can't be in the ZCT: process_zct ran just before us, and
  // anything in the current epoch with refcount 0 was freed there.
  for (size_t i = 0; i < garbage.len; i++) {
    GCHeader *h = garbage.items[i];
    gc_kind_of(h)->traverse(h, release_child, NULL);
  }
  for (size_t i = 0; i < garbage.len; i++)
    destroy(garbage.items[i]);
  stats.freed_by_cycle += garbage.len;
  gc_vec_free(&garbage);

  double pause = now_ms() - t0;
  last_cycle_pause_ms = pause;
  stats.cycle_collections++;
  stats.cycle_pause_ms_total += pause;
  if (pause > stats.cycle_pause_ms_max)
    stats.cycle_pause_ms_max = pause;
}

// ---------------------------------------------------------------------------
// Safe points
// ---------------------------------------------------------------------------

static void run_safe_point(bool full) {
  if (in_collection)
    return;
  in_collection = true;

  process_zct();
  if (full) {
    collect_cycles();
    process_zct(); // objects released by the cycle collector
    heap_trim();
    allocs_since_cycle = 0;
    cycle_threshold = stats.live > CYCLE_MIN ? stats.live : CYCLE_MIN;
    if (gc_config.print_stats) {
      HeapStats hs = heap_stats();
      fprintf(stderr,
              "[gc] cycle collection #%zu: %.2f ms, %zu live objects, "
              "%zu KiB heap\n",
              stats.cycle_collections, last_cycle_pause_ms, stats.live,
              hs.mapped_bytes / 1024);
    }
    if (gc_config.verify)
      gc_verify();
  }

  stress_pending = false;
  gc_work_due = zct_len >= zct_limit || allocs_since_cycle >= cycle_threshold;
  in_collection = false;
}

void gc_safe_point_slow(void) {
  run_safe_point(stress_pending || allocs_since_cycle >= cycle_threshold);
}

void gc_collect_now(void) { run_safe_point(true); }

// ---------------------------------------------------------------------------
// Setup and stats
// ---------------------------------------------------------------------------

static bool env_flag(const char *name) {
  const char *v = getenv(name);
  return v && *v && strcmp(v, "0") != 0;
}

void gc_init(void) {
  heap_init();
  gc_config.print_stats = env_flag("ARGON_GC_STATS");
  gc_config.verify = env_flag("ARGON_GC_VERIFY");
  gc_config.nofree = env_flag("ARGON_GC_NOFREE");
  const char *stress = getenv("ARGON_GC_STRESS");
  gc_config.stress_every = stress ? strtoull(stress, NULL, 10) : 0;
}

GCStats gc_stats(void) { return stats; }

void gc_shutdown(void) {
  if (!gc_config.print_stats)
    return;
  HeapStats hs = heap_stats();
  fprintf(stderr,
          "[gc] %zu allocated, %zu freed by refcount, %zu freed by cycle "
          "collection, %zu live; %zu cycle collections (%.2f ms total, %.2f "
          "ms max); heap %zu KiB\n",
          stats.allocated, stats.freed_by_rc, stats.freed_by_cycle, stats.live,
          stats.cycle_collections, stats.cycle_pause_ms_total,
          stats.cycle_pause_ms_max, hs.mapped_bytes / 1024);
}

#endif // ARGON_GC_CUSTOM
