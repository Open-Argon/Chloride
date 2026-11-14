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
#include <pthread.h>
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
  struct {
    gc_epoch epoch;
    int state;
  } blocking_saved[MAX_BLOCKING_DEPTH];
  struct GCThread *prev, *next;
} GCThread;

bool gc_multithreaded;
static size_t live_threads; // extra threads started but not yet finished
static pthread_mutex_t alloc_lock = PTHREAD_MUTEX_INITIALIZER;

static GCThread main_thread = {.state = GC_THREAD_RUNNING};
static GCThread *threads = &main_thread; // the registry
static __thread GCThread *self;          // this thread's record
static pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t parked_cv = PTHREAD_COND_INITIALIZER;
static pthread_cond_t resume_cv = PTHREAD_COND_INITIALIZER;
static bool stop_requested; // guarded by threads_lock; read racily as a hint
static size_t retry_after_alloc; // backoff after a stop attempt timed out

#define STOP_TIMEOUT_NS 2000000 // how long to wait for others to park: 2 ms
#define RETRY_ALLOCS 20000      // don't try again for this many allocations

static gc_thread_root_tracer *thread_root_tracers;
static size_t thread_root_tracer_count;

static uint64_t birth_counter; // birth number of the next allocation

static uint64_t current_birth(void) {
  return __atomic_load_n(&birth_counter, __ATOMIC_RELAXED);
}

// Threads not created through our pthread_create wrapper have no record;
// they get a private one so the API still works, but no collection ever
// knows about them (they mustn't touch objects).
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
  pthread_mutex_lock(&threads_lock); // waits for any collection in progress
  t->epoch = current_birth();
  t->next = threads;
  threads->prev = t;
  threads = t;
  pthread_mutex_lock(&alloc_lock);
  live_threads++;
  __atomic_store_n(&gc_multithreaded, true, __ATOMIC_RELEASE);
  pthread_mutex_unlock(&alloc_lock);
  pthread_mutex_unlock(&threads_lock);
  return t;
}

void gc_thread_adopt(void *thread) { self = thread; }

void gc_thread_finished(void *thread) {
  GCThread *t = thread;
  pthread_mutex_lock(&threads_lock);
  if (t->prev)
    t->prev->next = t->next;
  else
    threads = t->next;
  if (t->next)
    t->next->prev = t->prev;
  pthread_mutex_lock(&alloc_lock);
  if (--live_threads == 0) {
    __atomic_store_n(&gc_multithreaded, false, __ATOMIC_RELEASE);
    gc_work_due = true; // catch up on everything that died meanwhile
  }
  pthread_mutex_unlock(&alloc_lock);
  pthread_cond_broadcast(&parked_cv); // a collector may be waiting on it
  pthread_mutex_unlock(&threads_lock);
  if (self == t)
    self = NULL;
  free(t);
}

// Leaving BLOCKING: wait out any collection in progress first, since it may
// be freeing things this thread is about to look at. threads_lock held.
static void resume_running(GCThread *t, int state) {
  while (stop_requested)
    pthread_cond_wait(&resume_cv, &threads_lock);
  t->state = state;
}

void gc_thread_attach(void) {
  GCThread *t = me();
  pthread_mutex_lock(&threads_lock);
  resume_running(t, GC_THREAD_RUNNING);
  pthread_mutex_unlock(&threads_lock);
}

void gc_thread_detach(void) {
  GCThread *t = me();
  pthread_mutex_lock(&threads_lock);
  t->state = GC_THREAD_BLOCKING;
  pthread_cond_broadcast(&parked_cv);
  pthread_mutex_unlock(&threads_lock);
}

void gc_blocking_begin(void) {
  GCThread *t = me();
  pthread_mutex_lock(&threads_lock);
  if (t->blocking_depth < MAX_BLOCKING_DEPTH) {
    t->blocking_saved[t->blocking_depth].epoch = t->epoch;
    t->blocking_saved[t->blocking_depth].state = t->state;
  }
  t->blocking_depth++;
  // Whatever the blocked C code holds was born before now.
  t->epoch = current_birth();
  t->state = GC_THREAD_BLOCKING;
  pthread_cond_broadcast(&parked_cv);
  pthread_mutex_unlock(&threads_lock);
}

void gc_blocking_end(void) {
  GCThread *t = me();
  pthread_mutex_lock(&threads_lock);
  if (t->blocking_depth > 0 && --t->blocking_depth < MAX_BLOCKING_DEPTH) {
    t->epoch = t->blocking_saved[t->blocking_depth].epoch;
    resume_running(t, t->blocking_saved[t->blocking_depth].state);
  } else {
    resume_running(t, t->state);
  }
  pthread_mutex_unlock(&threads_lock);
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
  bool lock = gc_threads_running();
  if (lock)
    pthread_mutex_lock(&alloc_lock);
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
  if (lock)
    pthread_mutex_unlock(&alloc_lock);
  return h;
}

void gc_zero_count(GCHeader *h) {
  if (gc_threads_running()) {
    pthread_mutex_lock(&alloc_lock);
    if (!(h->flags & GC_IN_ZCT))
      zct_push(h);
    pthread_mutex_unlock(&alloc_lock);
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
    pthread_mutex_lock(&alloc_lock);
  h->flags &= ~GC_MORTAL; // the ZCT drops it next time it is processed
  if (lock)
    pthread_mutex_unlock(&alloc_lock);
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
  // Kept entries are re-examined every time; let the table grow in
  // proportion so that doesn't turn quadratic.
  zct_limit = 2 * kept + ZCT_MIN;
}

// ---------------------------------------------------------------------------
// Cycle collection (trial deletion)
// ---------------------------------------------------------------------------

// Step 0: candidates are mortal, live, container objects born in the current
// epoch. Their gc_refs start as their refcount, plus any references from
// uncounted roots (already there, see hold_root).
static void add_candidate(void *cell, void *ctx) {
  GCHeader *h = cell;
  if (!(h->flags & GC_MORTAL) || (h->flags & GC_ZOMBIE) ||
      h->birth < collect_epoch || !gc_kind_of(h)->traverse)
    return;
  h->flags |= GC_CANDIDATE;
  h->gc_refs += h->refcount;
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

// The work of a safe point. Runs with no other thread touching objects:
// either there is only one thread, or every other one is parked/blocking.
static void collect(bool full) {
  in_collection = true;

  trace_uncounted_roots(hold_root);
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
              "%zu KiB heap%s\n",
              stats.cycle_collections, last_cycle_pause_ms, stats.live,
              hs.mapped_bytes / 1024,
              gc_threads_running() ? " (threads stopped)" : "");
    }
  }
  trace_uncounted_roots(release_root);
  // The verifier walks every thread's roots too, but keep it simple: only
  // with a single thread (after release_root: it needs gc_refs to be 0).
  if (full && gc_config.verify && !gc_threads_running())
    gc_verify();

  stress_pending = false;
  gc_work_due = zct_len >= zct_limit || allocs_since_cycle >= cycle_threshold;
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
  pthread_cond_broadcast(&parked_cv);
  while (stop_requested)
    pthread_cond_wait(&resume_cv, &threads_lock);
  t->state = GC_THREAD_RUNNING;
}

// Collect with several threads: stop the others, collect, restart them.
static void collect_world(bool full) {
  GCThread *t = me();
  if (!self) // an unknown thread never collects
    return;
  pthread_mutex_lock(&threads_lock);
  if (stop_requested) { // someone else got there first
    park(t);
    pthread_mutex_unlock(&threads_lock);
    return;
  }

  stop_requested = true;
  gc_work_due = true; // sends every running thread into its slow path
  struct timespec deadline;
  clock_gettime(CLOCK_REALTIME, &deadline);
  deadline.tv_nsec += STOP_TIMEOUT_NS;
  if (deadline.tv_nsec >= 1000000000L) {
    deadline.tv_sec++;
    deadline.tv_nsec -= 1000000000L;
  }
  bool stopped = others_stopped();
  while (!stopped) {
    if (pthread_cond_timedwait(&parked_cv, &threads_lock, &deadline) != 0) {
      stopped = others_stopped();
      break; // timed out
    }
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
    collect(full);
  } else {
    // Someone is busy outside a safe point (a long native call, say). Don't
    // wait for them; try again a little later.
    retry_after_alloc = stats.allocated + RETRY_ALLOCS;
  }

  stop_requested = false;
  pthread_cond_broadcast(&resume_cv);
  pthread_mutex_unlock(&threads_lock);
}

static void run_safe_point(bool full) {
  if (in_collection)
    return;
  // ACQUIRE pairs with gc_thread_finished: once we see no other threads, we
  // also see every count update they made.
  if (!__atomic_load_n(&gc_multithreaded, __ATOMIC_ACQUIRE)) {
    collect_epoch = me()->epoch;
    collect(full);
    return;
  }
  collect_world(full);
}

void gc_safe_point_slow(void) {
  if (__atomic_load_n(&gc_multithreaded, __ATOMIC_ACQUIRE)) {
    // Asked to stop for someone else's collection?
    if (__atomic_load_n(&stop_requested, __ATOMIC_ACQUIRE) && self) {
      pthread_mutex_lock(&threads_lock);
      if (stop_requested)
        park(self);
      pthread_mutex_unlock(&threads_lock);
      return;
    }
    // Backing off after a stop attempt that timed out.
    if (__atomic_load_n(&stats.allocated, __ATOMIC_RELAXED) <
        __atomic_load_n(&retry_after_alloc, __ATOMIC_RELAXED))
      return;
  }
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
          "collection, %zu live; %zu cycle collections (%.2f ms total, %.2f "
          "ms max); heap %zu KiB\n",
          stats.allocated, stats.freed_by_rc, stats.freed_by_cycle, stats.live,
          stats.cycle_collections, stats.cycle_pause_ms_total,
          stats.cycle_pause_ms_max, hs.mapped_bytes / 1024);
}

#endif // ARGON_GC_CUSTOM
