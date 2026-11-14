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
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Process the ZCT once it holds this many entries (more than the ones we had
// to keep last time). Small = prompt freeing, large = less overhead.
#define ZCT_MIN 1024
// Cycle collection is generational (README.md, "Generations"):
//  * a YOUNG collection every YOUNG_THRESHOLD allocations looks only at
//    objects allocated since the last cycle collection;
//  * a FULL collection walks the whole heap, once objects that survived
//    young collections since the last full one reach a quarter of what was
//    live after it (and at least FULL_MIN), so its cost per allocation stays
//    constant however big the heap gets.
#define YOUNG_THRESHOLD 10000
#define FULL_MIN 50000

// What a safe point should do besides processing the ZCT.
enum { CYCLES_NONE, CYCLES_DUE, CYCLES_FULL };

static void *xrealloc(void *p, size_t size);

GCConfig gc_config;
const GCKind *gc_kinds[256];
size_t gc_kind_count;
GCHeader **gc_statics;
size_t gc_static_count;
gc_root_tracer *gc_counted_roots, *gc_uncounted_roots;
size_t gc_counted_root_count, gc_uncounted_root_count;

bool gc_work_due;

// ---------------------------------------------------------------------------
// Threads (stage 6). See README.md, "Threads".
//
// Every thread that may touch objects has a GCThread record, in one of three
// states:
//   RUNNING   executing; may touch objects at any moment
//   PARKED    stopped at a safe point because a collection was requested
//   BLOCKING  promised not to touch objects: inside gc_blocking_begin/end,
//             or not (yet / any more) registered with the runtime
// A collection needs every other thread PARKED or BLOCKING. The collecting
// thread asks (stop_requested), waits a short while, and gives up if someone
// doesn't stop in time (a long native call, say) - so an unmarked native
// can only delay collection, never deadlock it.
//
// While more than one thread exists, refcount updates are atomic (gc.h) and
// allocation and the ZCT take alloc_lock.
// ---------------------------------------------------------------------------

enum { GC_THREAD_RUNNING, GC_THREAD_PARKED, GC_THREAD_BLOCKING };

#define MAX_BLOCKING_DEPTH 16

typedef struct GCThread {
  int state;
  // The innermost epoch of this thread: its innermost runtime() invocation,
  // or blocking region. Objects older than this may be held by its C code.
  gc_epoch epoch;
  void *roots; // for gc_add_thread_roots tracers (runtime.c's frame list)
  int blocking_depth;
  bool self_registered; // made by gc_thread_attach, not gc_thread_started
  struct {
    gc_epoch epoch;
    int state;
  } blocking_saved[MAX_BLOCKING_DEPTH];
  struct GCThread *prev, *next;
} GCThread;

bool gc_multithreaded;
static size_t live_threads; // extra threads started but not yet finished
static gc_mutex alloc_lock = GC_MUTEX_INIT;

static GCThread main_thread = {.state = GC_THREAD_RUNNING};
static GCThread *threads = &main_thread; // the registry
static __thread GCThread *self;          // this thread's record
static gc_mutex threads_lock = GC_MUTEX_INIT;
static gc_cond parked_cv = GC_COND_INIT;
static gc_cond resume_cv = GC_COND_INIT;
static bool stop_requested; // guarded by threads_lock; read racily as a hint
static size_t retry_after_alloc; // backoff after a stop attempt timed out

#define STOP_TIMEOUT_MS 2.0 // how long to wait for others to park
#define RETRY_ALLOCS 20000      // don't try again for this many allocations

static gc_thread_root_tracer *thread_root_tracers;
static size_t thread_root_tracer_count;

static uint64_t birth_counter; // birth number of the next allocation

static uint64_t current_birth(void) {
  return __atomic_load_n(&birth_counter, __ATOMIC_RELAXED);
}

// Threads not started through ar_prepare_thread have no record until they
// call the native API's register_thread (see gc_thread_attach); until then
// they get a private one, which no collection knows about.
static __thread GCThread unknown_thread;
static GCThread *me(void) { return self ? self : &unknown_thread; }

void *gc_thread_started(void) {
  GCThread *t = calloc(1, sizeof *t);
  if (!t) {
    fprintf(stderr, "panic: gc: out of memory\n");
    exit(EXIT_FAILURE);
  }
  t->state = GC_THREAD_BLOCKING;
  // Everything that exists now may have been handed to the new thread, and
  // nothing counts those references: protect it all until the thread enters
  // runtime() (whose epoch is later) or finishes.
  gc_mutex_lock(&threads_lock); // waits for any collection in progress
  t->epoch = current_birth();
  t->next = threads;
  threads->prev = t;
  threads = t;
  gc_mutex_lock(&alloc_lock);
  live_threads++;
  __atomic_store_n(&gc_multithreaded, true, __ATOMIC_RELEASE);
  gc_mutex_unlock(&alloc_lock);
  gc_mutex_unlock(&threads_lock);
  return t;
}

void gc_thread_adopt(void *thread) { self = thread; }

void gc_thread_finished(void *thread) {
  GCThread *t = thread;
  gc_mutex_lock(&threads_lock);
  if (t->prev)
    t->prev->next = t->next;
  else
    threads = t->next;
  if (t->next)
    t->next->prev = t->prev;
  gc_mutex_lock(&alloc_lock);
  if (--live_threads == 0) {
    __atomic_store_n(&gc_multithreaded, false, __ATOMIC_RELEASE);
    gc_work_due = true; // catch up on everything that died meanwhile
  }
  gc_mutex_unlock(&alloc_lock);
  gc_cond_broadcast(&parked_cv); // a collector may be waiting on it
  gc_mutex_unlock(&threads_lock);
  if (self == t)
    self = NULL;
  free(t);
}

// Leaving BLOCKING: wait out any collection in progress first, since it may
// be freeing things this thread is about to look at. threads_lock held.
static void resume_running(GCThread *t, int state) {
  while (stop_requested)
    gc_cond_wait(&resume_cv, &threads_lock);
  t->state = state;
}

void gc_thread_attach(void) {
  if (!self) {
    // A thread we weren't told about (its creator didn't use
    // ar_prepare_thread): register it now. It's protected from here on,
    // though not in the moment between its creation and this call.
    GCThread *t = gc_thread_started();
    t->self_registered = true;
    self = t;
  }
  GCThread *t = me();
  gc_mutex_lock(&threads_lock);
  resume_running(t, GC_THREAD_RUNNING);
  gc_mutex_unlock(&threads_lock);
}

void gc_thread_detach(void) {
  GCThread *t = me();
  gc_mutex_lock(&threads_lock);
  t->state = GC_THREAD_BLOCKING;
  gc_cond_broadcast(&parked_cv);
  gc_mutex_unlock(&threads_lock);
  if (t->self_registered) // nobody else will finish it for us
    gc_thread_finished(t);
}

void gc_blocking_begin(void) {
  GCThread *t = me();
  gc_mutex_lock(&threads_lock);
  if (t->blocking_depth < MAX_BLOCKING_DEPTH) {
    t->blocking_saved[t->blocking_depth].epoch = t->epoch;
    t->blocking_saved[t->blocking_depth].state = t->state;
  }
  t->blocking_depth++;
  // Whatever the blocked C code holds was born before now.
  t->epoch = current_birth();
  t->state = GC_THREAD_BLOCKING;
  gc_cond_broadcast(&parked_cv);
  gc_mutex_unlock(&threads_lock);
}

void gc_blocking_end(void) {
  GCThread *t = me();
  gc_mutex_lock(&threads_lock);
  if (t->blocking_depth > 0 && --t->blocking_depth < MAX_BLOCKING_DEPTH) {
    t->epoch = t->blocking_saved[t->blocking_depth].epoch;
    resume_running(t, t->blocking_saved[t->blocking_depth].state);
  } else {
    resume_running(t, t->state);
  }
  gc_mutex_unlock(&threads_lock);
}

void **gc_thread_roots(void) { return &me()->roots; }

void gc_add_thread_roots(gc_thread_root_tracer tracer) {
  thread_root_tracers =
      xrealloc(thread_root_tracers,
                     (thread_root_tracer_count + 1) * sizeof *thread_root_tracers);
  thread_root_tracers[thread_root_tracer_count++] = tracer;
}

static bool in_collection;      // guards against re-entering a safe point
// The epoch a collection in progress uses: the current thread's, or with
// several threads, the latest of all of theirs (see collect_world).
static gc_epoch collect_epoch;
static bool stress_pending;     // ARGON_GC_STRESS asked for a full safe point
static size_t stress_counter;

// The zero count table: objects whose refcount is (or was, when listed) 0.
static GCHeader **zct;
static size_t zct_len, zct_cap;
static size_t zct_limit = ZCT_MIN;
// The ZCT is also processed after this many allocations, however short it
// is: one entry can be a big dead structure (an array of a million numbers).
// It's the larger of ZCT_MIN and the entries the last pass had to keep, so
// those are re-examined at most once per that many allocations.
static size_t zct_alloc_limit = ZCT_MIN;
static size_t allocs_since_zct;

static size_t allocs_since_cycle;
// Every object allocated since the last cycle collection (plus some older
// ones the epoch rule kept out of it): the young generation's candidates.
// Entries for objects freed meanwhile are spotted with heap_cell_of.
static GCVec young;
static size_t promoted_since_full; // young survivors since the last full one
static size_t live_after_full;

static GCStats stats;
static double last_cycle_pause_ms;

static double now_ms(void) { return gc_now_ms(); }

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

static void *allocate(uint8_t kind, size_t size, bool young_candidate);

void *gc_new(uint8_t kind, size_t size) { return allocate(kind, size, true); }

void *gc_new_leaf(uint8_t kind, size_t size) {
  return allocate(kind, size, false);
}

static void *allocate(uint8_t kind, size_t size, bool young_candidate) {
  if (kind >= gc_kind_count) {
    fprintf(stderr, "panic: gc: gc_new with unregistered kind %u\n", kind);
    abort();
  }
  bool lock = gc_threads_running();
  if (lock)
    gc_mutex_lock(&alloc_lock);
  GCHeader *h = heap_alloc(size); // zeroed: refcount 0, gc_refs 0
  h->kind = kind;
  h->flags = GC_MORTAL;
  h->birth = birth_counter++;
  stats.allocated++;
  stats.live++;

  // A new object has no counted references yet. If nothing stores it before
  // the next safe point, it's garbage, so it starts out in the ZCT.
  zct_push(h);

  if (young_candidate)
    gc_vec_push(&young, h);
  if (++allocs_since_zct >= zct_alloc_limit)
    gc_work_due = true;
  if (++allocs_since_cycle >= YOUNG_THRESHOLD)
    gc_work_due = true;
  if (gc_config.stress_every && ++stress_counter >= gc_config.stress_every) {
    stress_counter = 0;
    stress_pending = true;
    gc_work_due = true;
  }
  if (lock)
    gc_mutex_unlock(&alloc_lock);
  return h;
}

// A new object starts in the ZCT, and is usually stored somewhere straight
// away, while it's still the last entry. Then it can come straight back out,
// so the ZCT doesn't fill up with (and later re-examine, from cold memory)
// objects that were never garbage. Single-threaded only, and not while a
// safe point is walking the table.
void gc_first_reference(GCHeader *h) {
  if (!in_collection && zct_len && zct[zct_len - 1] == h) {
    zct_len--;
    h->flags &= ~GC_IN_ZCT;
  }
}

void gc_zero_count(GCHeader *h) {
  if (gc_threads_running()) {
    gc_mutex_lock(&alloc_lock);
    if (!(h->flags & GC_IN_ZCT))
      zct_push(h);
    gc_mutex_unlock(&alloc_lock);
    return;
  }
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

size_t gc_object_size(const void *obj) { return heap_cell_size(obj); }

void gc_immortalize(void *obj) {
  GCHeader *h = obj;
  if (!h)
    return;
  // flags share a word with other header bits that zct_push may be setting
  // on another thread, so change them under the lock while threads run.
  bool lock = gc_threads_running();
  if (lock)
    gc_mutex_lock(&alloc_lock);
  h->flags &= ~GC_MORTAL; // the ZCT drops it next time it is processed
  if (lock)
    gc_mutex_unlock(&alloc_lock);
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

static void add_tracer(gc_root_tracer **list, size_t *count,
                       gc_root_tracer tracer) {
  *list = xrealloc(*list, (*count + 1) * sizeof **list);
  (*list)[(*count)++] = tracer;
}

void gc_add_counted_roots(gc_root_tracer tracer) {
  add_tracer(&gc_counted_roots, &gc_counted_root_count, tracer);
}

void gc_add_uncounted_roots(gc_root_tracer tracer) {
  add_tracer(&gc_uncounted_roots, &gc_uncounted_root_count, tracer);
}

// ---------------------------------------------------------------------------
// Uncounted roots
//
// At the start of a safe point every object an uncounted root points at gets
// GC_ROOTED, and gc_refs counts how many such references it has (the cycle
// collector adds its refcount on top, so rooted objects always look
// externally referenced). At the end, both are cleared again.
// ---------------------------------------------------------------------------

static void hold_root(GCHeader *obj, void *ctx) {
  (void)ctx;
  if (obj && (obj->flags & GC_MORTAL)) {
    obj->flags |= GC_ROOTED;
    obj->gc_refs++;
  }
}

static void release_root(GCHeader *obj, void *ctx) {
  (void)ctx;
  if (obj && (obj->flags & GC_MORTAL)) {
    obj->flags &= ~GC_ROOTED;
    obj->gc_refs = 0;
  }
}

// Global uncounted roots, then every registered thread's own (its frames).
static void trace_uncounted_roots(gc_visit_fn visit) {
  for (size_t i = 0; i < gc_uncounted_root_count; i++)
    gc_uncounted_roots[i](visit, NULL);
  for (GCThread *t = threads; t; t = t->next)
    for (size_t i = 0; i < thread_root_tracer_count; i++)
      thread_root_tracers[i](t->roots, visit, NULL);
}

void gc_trace_all_uncounted_roots(gc_visit_fn visit, void *ctx) {
  for (size_t i = 0; i < gc_uncounted_root_count; i++)
    gc_uncounted_roots[i](visit, ctx);
  for (GCThread *t = threads; t; t = t->next)
    for (size_t i = 0; i < thread_root_tracer_count; i++)
      thread_root_tracers[i](t->roots, visit, ctx);
}

// ---------------------------------------------------------------------------
// Epochs
// ---------------------------------------------------------------------------

// Only this thread writes its own epoch; a collector reads it only while
// this thread is parked or blocking.
gc_epoch gc_enter(void) {
  GCThread *t = me();
  gc_epoch previous = t->epoch;
  t->epoch = current_birth();
  return previous;
}

void gc_leave(gc_epoch previous) { me()->epoch = previous; }

// ---------------------------------------------------------------------------
// Freeing
// ---------------------------------------------------------------------------

static void free_object(GCHeader *h);

// Drop the reference a dying object holds on `child`. Children that are
// themselves part of a garbage cycle being freed are skipped: they're going
// away regardless, and their counts no longer matter.
//
// This only runs inside a safe point (other threads, if any, are stopped),
// so the decref can be a plain one, and a child that dies here can often be
// freed on the spot instead of going through the ZCT: that saves a second
// trip to memory that has long left the cache (freeing an array of a million
// numbers, say). That's only done for acyclic children (whose own children
// are all immortal, so it can't recurse) that nothing else protects: not
// listed in the ZCT (the entry would dangle), not held by a root, and born
// in the current epoch.
static void release_child(GCHeader *child, void *ctx) {
  (void)ctx;
  if (!child || (child->flags & GC_DYING) || !(child->flags & GC_MORTAL))
    return;
  if (child->refcount == 0) {
    gc_bad_decref(child);
    return;
  }
  if (--child->refcount > 0)
    return;
  const GCKind *k = gc_kind_of(child);
  if (!(child->flags & (GC_IN_ZCT | GC_ROOTED | GC_ZOMBIE)) &&
      child->birth >= collect_epoch && k->acyclic && k->acyclic(child)) {
    stats.freed_by_rc++;
    free_object(child);
  } else {
    gc_zero_count(child);
  }
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
//   * refcount 0 but held by an uncounted root (a VM register, say): keep it;
//     it dies once the root lets go of it
//   * dead but older than the current epoch: a suspended native might still
//     be holding it, so keep it for a shallower safe point
// Freeing an object decrefs its children, which may append them to the ZCT;
// the loop picks those up too, so whole structures go in one pass with no
// recursion.
static void process_zct(void) {
  size_t kept = 0;
  for (size_t i = 0; i < zct_len; i++) {
    GCHeader *h = zct[i];
    if (h->flags & GC_DYING) {
      // Garbage the cycle collector found while it was still listed here:
      // its references are already released; only the memory is left.
      h->flags &= ~GC_IN_ZCT;
      destroy(h);
      continue;
    }
    if (h->refcount > 0 || !(h->flags & GC_MORTAL) ||
        (h->flags & GC_ZOMBIE)) {
      h->flags &= ~GC_IN_ZCT;
    } else if (h->birth >= collect_epoch && !(h->flags & GC_ROOTED)) {
      h->flags &= ~GC_IN_ZCT;
      stats.freed_by_rc++;
      free_object(h);
    } else {
      zct[kept++] = h; // kept <= i, so this never overwrites unread entries
    }
  }
  zct_len = kept;
  // Kept entries are re-examined every time; let the table (and the time
  // between passes) grow in proportion so that doesn't turn quadratic.
  zct_limit = 2 * kept + ZCT_MIN;
  zct_alloc_limit = kept > ZCT_MIN ? kept : ZCT_MIN;
  allocs_since_zct = 0;
}

static bool zct_due(void) {
  return zct_len >= zct_limit || allocs_since_zct >= zct_alloc_limit;
}

// ---------------------------------------------------------------------------
// Cycle collection (trial deletion)
// ---------------------------------------------------------------------------

// Step 0: candidates are mortal, live, container objects born in the current
// epoch. Their gc_refs start as their refcount, plus any references from
// uncounted roots (already there, see hold_root).
static void add_candidate(void *cell, void *ctx) {
  GCHeader *h = cell;
  const GCKind *k = gc_kind_of(h);
  if (!(h->flags & GC_MORTAL) ||
      (h->flags & (GC_ZOMBIE | GC_CANDIDATE | GC_DYING)) ||
      h->birth < collect_epoch || !k->traverse || (k->acyclic && k->acyclic(h)))
    return;
  h->flags |= GC_CANDIDATE;
  h->gc_refs += h->refcount;
  gc_vec_push(ctx, h);
}

#define GC_REFS_BROKEN 0x80000000u

// Is `child` a candidate? Normally that's the GC_CANDIDATE flag. But a young
// collection has few candidates, and checking the flag means reading the
// header of every child of every candidate - for an array of a million
// numbers, a million cache misses to learn that none of them is one. So a
// young collection also puts its candidates in a small hash set of
// addresses, which answers without touching the child at all.
static struct {
  GCHeader **slots; // NULL when not in use
  size_t mask;
} candidate_set;

static size_t candidate_hash(const GCHeader *p) {
  return (size_t)(((uintptr_t)p >> 4) * 0x9E3779B97F4A7C15ull >> 20);
}

static void candidate_set_build(GCVec *candidates) {
  size_t cap = 64;
  while (cap < 2 * candidates->len)
    cap *= 2;
  candidate_set.slots = calloc(cap, sizeof(GCHeader *));
  if (!candidate_set.slots)
    return; // fall back to the flag
  candidate_set.mask = cap - 1;
  for (size_t i = 0; i < candidates->len; i++) {
    size_t j = candidate_hash(candidates->items[i]) & candidate_set.mask;
    while (candidate_set.slots[j])
      j = (j + 1) & candidate_set.mask;
    candidate_set.slots[j] = candidates->items[i];
  }
}

static void candidate_set_free(void) {
  free(candidate_set.slots);
  candidate_set.slots = NULL;
}

static inline bool is_candidate(GCHeader *child) {
  if (!child)
    return false;
  if (!candidate_set.slots)
    return child->flags & GC_CANDIDATE;
  for (size_t j = candidate_hash(child) & candidate_set.mask;
       candidate_set.slots[j]; j = (j + 1) & candidate_set.mask)
    if (candidate_set.slots[j] == child)
      return true;
  return false;
}

// Step 1: remove references that come from inside the candidate set.
static void subtract_internal(GCHeader *child, void *ctx) {
  (void)ctx;
  if (!is_candidate(child))
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
  if (is_candidate(child) && !(child->flags & GC_REACHABLE)) {
    child->flags |= GC_REACHABLE;
    gc_vec_push(ctx, child);
  }
}

// Empty the young list, and (for a young collection) take its candidates
// from it, skipping entries for objects freed since. Objects the epoch rule
// protects right now aren't candidates; like the survivors they become old
// (counted as promoted), and a full collection deals with them later.
// (Keeping them for the next young collection instead made each one rescan
// all of them: inside a long native call that's most of the heap.)
static size_t scan_young(GCVec *candidates) {
  size_t protected_now = 0;
  for (size_t i = 0; i < young.len; i++) {
    GCHeader *h = young.items[i];
    if (heap_cell_of(h) != h) // freed (a reused cell is a newer object,
      continue;               // which has its own entry: no harm either way)
    if (!candidates)
      continue;
    if (h->birth < collect_epoch && (h->flags & GC_MORTAL) &&
        !(h->flags & GC_ZOMBIE))
      protected_now++;
    else
      add_candidate(h, candidates);
  }
  young.len = 0;
  return protected_now;
}

// Returns true if some garbage was left in the ZCT (DYING) to be freed there.
static bool collect_cycles(bool full) {
  double t0 = now_ms();

  // Step 0: the candidates. A young collection only looks at recent
  // objects; references to them from older objects count as coming from
  // outside, which is safe (anything an old object points at is kept).
  GCVec candidates = {0};
  size_t protected_now = 0;
  if (full) {
    heap_for_each(add_candidate, &candidates);
    scan_young(NULL);
  } else {
    protected_now = scan_young(&candidates);
  }

  if (!full)
    candidate_set_build(&candidates);
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
  candidate_set_free();

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
  size_t candidates_examined = candidates.len;
  gc_vec_free(&candidates);

  // Step 4: free it. First release every reference from garbage to
  // non-garbage (those objects may die too, via the ZCT), then free memory.
  // A garbage object still listed in the ZCT can't be freed yet (the ZCT
  // would point at a freed cell): it stays DYING, and the next ZCT pass
  // frees it. That way a collection doesn't have to process the ZCT first.
  for (size_t i = 0; i < garbage.len; i++) {
    GCHeader *h = garbage.items[i];
    gc_kind_of(h)->traverse(h, release_child, NULL);
  }
  bool left_in_zct = false;
  for (size_t i = 0; i < garbage.len; i++) {
    if (garbage.items[i]->flags & GC_IN_ZCT)
      left_in_zct = true;
    else
      destroy(garbage.items[i]);
  }
  stats.freed_by_cycle += garbage.len;
  if (full) {
    promoted_since_full = 0;
  } else {
    // survivors, plus young objects the epoch rule kept out of this one
    promoted_since_full += candidates_examined - garbage.len + protected_now;
    stats.young_collections++;
  }
  gc_vec_free(&garbage);

  double pause = now_ms() - t0;
  last_cycle_pause_ms = pause;
  stats.cycle_collections++;
  stats.cycle_pause_ms_total += pause;
  if (pause > stats.cycle_pause_ms_max)
    stats.cycle_pause_ms_max = pause;
  return left_in_zct;
}

// ---------------------------------------------------------------------------
// Safe points
// ---------------------------------------------------------------------------

// The work of a safe point. Runs with no other thread touching objects:
// either there is only one thread, or every other one is parked/blocking.
static bool full_collection_due(void) {
  size_t limit = live_after_full / 4;
  return promoted_since_full >= (limit > FULL_MIN ? limit : FULL_MIN);
}

// The work of a safe point. Runs with no other thread touching objects:
// either there is only one thread, or every other one is parked/blocking.
static void collect(int cycles) {
  in_collection = true;

  bool full = cycles == CYCLES_FULL ||
              (cycles == CYCLES_DUE && full_collection_due());
  trace_uncounted_roots(hold_root);
  // A ZCT pass re-examines the entries the epoch rule or a root keeps
  // alive, so only make one when it's due (or for a full collection), not
  // just because a young cycle collection is.
  if (full || zct_due())
    process_zct();
  if (cycles != CYCLES_NONE) {
    // Garbage it finds that's still listed in the ZCT is left DYING for a
    // ZCT pass to free; make sure one sees it now.
    if (collect_cycles(full) || full)
      process_zct();
    allocs_since_cycle = 0;
    if (full) {
      heap_trim();
      live_after_full = stats.live;
    }
    if (gc_config.print_stats) {
      HeapStats hs = heap_stats();
      fprintf(stderr,
              "[gc] %s cycle collection #%zu: %.2f ms, %zu live objects, "
              "%zu KiB heap%s\n",
              full ? "full" : "young", stats.cycle_collections,
              last_cycle_pause_ms, stats.live, hs.mapped_bytes / 1024,
              gc_threads_running() ? " (threads stopped)" : "");
    }
  }
  trace_uncounted_roots(release_root);
  // The verifier walks every thread's roots too, but keep it simple: only
  // with a single thread (after release_root: it needs gc_refs to be 0).
  if (cycles != CYCLES_NONE && gc_config.verify && !gc_threads_running())
    gc_verify();

  stress_pending = false;
  gc_work_due = zct_due() || allocs_since_cycle >= YOUNG_THRESHOLD;
  in_collection = false;
}

static bool others_stopped(void) {
  for (GCThread *t = threads; t; t = t->next)
    if (t != self && t->state == GC_THREAD_RUNNING)
      return false;
  return true;
}

// Called with threads_lock held, when another thread has asked to collect.
static void park(GCThread *t) {
  t->state = GC_THREAD_PARKED;
  gc_cond_broadcast(&parked_cv);
  while (stop_requested)
    gc_cond_wait(&resume_cv, &threads_lock);
  t->state = GC_THREAD_RUNNING;
}

// Collect with several threads: stop the others, collect, restart them.
static void collect_world(int cycles) {
  GCThread *t = me();
  if (!self) // an unknown thread never collects
    return;
  gc_mutex_lock(&threads_lock);
  if (stop_requested) { // someone else got there first
    park(t);
    gc_mutex_unlock(&threads_lock);
    return;
  }

  stop_requested = true;
  gc_work_due = true; // sends every running thread into its slow path
  double deadline = now_ms() + STOP_TIMEOUT_MS;
  bool stopped = others_stopped();
  while (!stopped) {
    double left = deadline - now_ms();
    if (left <= 0)
      break; // timed out
    gc_cond_wait_ms(&parked_cv, &threads_lock, left);
    stopped = others_stopped();
  }

  if (stopped) {
    // The epoch rule across threads: an object may be freed only if no
    // thread's suspended C code could be holding it, i.e. it was born after
    // every thread's current epoch.
    collect_epoch = 0;
    for (GCThread *u = threads; u; u = u->next)
      if (u->epoch > collect_epoch)
        collect_epoch = u->epoch;
    collect(cycles);
  } else {
    // Someone is busy outside a safe point (a long native call, say). Don't
    // wait for them; try again a little later.
    retry_after_alloc = stats.allocated + RETRY_ALLOCS;
  }

  stop_requested = false;
  gc_cond_broadcast(&resume_cv);
  gc_mutex_unlock(&threads_lock);
}

static void run_safe_point(int cycles) {
  if (in_collection)
    return;
  // ACQUIRE pairs with gc_thread_finished: once we see no other threads, we
  // also see every count update they made.
  if (!__atomic_load_n(&gc_multithreaded, __ATOMIC_ACQUIRE)) {
    collect_epoch = me()->epoch;
    collect(cycles);
    return;
  }
  collect_world(cycles);
}

void gc_safe_point_slow(void) {
  if (__atomic_load_n(&gc_multithreaded, __ATOMIC_ACQUIRE)) {
    // Asked to stop for someone else's collection?
    if (__atomic_load_n(&stop_requested, __ATOMIC_ACQUIRE) && self) {
      gc_mutex_lock(&threads_lock);
      if (stop_requested)
        park(self);
      gc_mutex_unlock(&threads_lock);
      return;
    }
    // Backing off after a stop attempt that timed out.
    if (__atomic_load_n(&stats.allocated, __ATOMIC_RELAXED) <
        __atomic_load_n(&retry_after_alloc, __ATOMIC_RELAXED))
      return;
  }
  run_safe_point(stress_pending ? CYCLES_FULL
                 : allocs_since_cycle >= YOUNG_THRESHOLD ? CYCLES_DUE
                                                         : CYCLES_NONE);
}

void gc_collect_now(void) { run_safe_point(CYCLES_FULL); }

// ---------------------------------------------------------------------------
// Setup and stats
// ---------------------------------------------------------------------------

static bool env_flag(const char *name) {
  const char *v = getenv(name);
  return v && *v && strcmp(v, "0") != 0;
}

void gc_init(void) {
  heap_init();
  self = &main_thread;
  gc_config.print_stats = env_flag("ARGON_GC_STATS");
  gc_config.verify = env_flag("ARGON_GC_VERIFY");
  gc_config.nofree = env_flag("ARGON_GC_NOFREE");
  const char *stress = getenv("ARGON_GC_STRESS");
  gc_config.stress_every = stress ? strtoull(stress, NULL, 10) : 0;
}

GCStats gc_stats(void) { return stats; }

void gc_shutdown(void) {
  if (!gc_config.print_stats && !gc_config.verify)
    return;
  // The program has finished, so no frame or native holds anything any more:
  // collect once more so "live" shows only what is really still referenced
  // (module globals and immortal objects), which makes leaks stand out.
  gc_collect_now();
  if (gc_config.verify && !gc_threads_running())
    gc_verify();
  if (!gc_config.print_stats)
    return;
  HeapStats hs = heap_stats();
  fprintf(stderr,
          "[gc] %zu allocated, %zu freed by refcount, %zu freed by cycle "
          "collection, %zu live; %zu cycle collections (%zu young, %zu full; "
          "%.2f ms total, %.2f ms max); heap %zu KiB\n",
          stats.allocated, stats.freed_by_rc, stats.freed_by_cycle, stats.live,
          stats.cycle_collections, stats.young_collections,
          stats.cycle_collections - stats.young_collections,
          stats.cycle_pause_ms_total, stats.cycle_pause_ms_max,
          hs.mapped_bytes / 1024);
}

#endif // ARGON_GC_CUSTOM
