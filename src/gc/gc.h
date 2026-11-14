/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Reference counting + cycle collection for managed objects.
// Read README.md first: this API only makes sense with the rules there.
//
// A *managed* type is a struct whose first member is a GCHeader:
//
//     struct Thing { GCHeader gc; ArgonObject *a, *b; char *owned_buffer; };
//
// Each managed type registers a GCKind telling the collector how to visit the
// counted pointers it holds (traverse) and how to free what it owns
// (finalize). Then:
//
//     struct Thing *t = gc_new(THING_KIND, sizeof *t);  // refcount 0, in ZCT
//     gc_assign(&holder->field, t);  // incref t, decref the old value
//
// With the Boehm backend all of this still compiles: gc_new allocates from
// Boehm and every count/collect operation is a no-op.

#ifndef ARGON_GC_H
#define ARGON_GC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// The header at the start of every managed object (16 bytes)
// ---------------------------------------------------------------------------

typedef struct GCHeader {
  uint32_t refcount; // counted references (see README: "counted holders")
  uint32_t gc_refs;  // scratch space for trial deletion and the verifier
  uint64_t birth : 48; // allocation number, for the epoch rule
  uint64_t kind : 8;   // index into the registered GCKinds
  uint64_t flags : 8;  // GC_* flags below
} GCHeader;

enum {
  // Set by gc_new. A header without it (e.g. a zero-initialised static
  // object) is immortal: never counted, never freed.
  GC_MORTAL = 1 << 0,
  GC_IN_ZCT = 1 << 1,   // currently listed in the zero count table
  GC_ZOMBIE = 1 << 2,   // "freed" while ARGON_GC_NOFREE is on (debugging)
  GC_CANDIDATE = 1 << 3, // scratch: in the current cycle collection's set
  GC_REACHABLE = 1 << 4, // scratch: proven reachable from outside the set
  GC_DYING = 1 << 5,     // scratch: in a garbage cycle being freed
  GC_ROOTED = 1 << 6,    // scratch: held by an uncounted root (safe point)
};

// ---------------------------------------------------------------------------
// Kinds
// ---------------------------------------------------------------------------

// Called once for every counted pointer an object holds. `child` may be NULL.
typedef void (*gc_visit_fn)(GCHeader *child, void *ctx);

typedef struct GCKind {
  const char *name; // shown in stats and verifier reports

  // Visit every *counted* pointer to a managed object that `obj` holds,
  // exactly once per reference (an object stored twice is visited twice).
  // NULL for kinds that never hold references (strings, numbers, ...).
  // This one function drives freeing, cycle detection and verification, so
  // it must agree exactly with where the type increfs.
  void (*traverse)(GCHeader *obj, gc_visit_fn visit, void *ctx);

  // Free memory the object owns (buffers, malloc'd arrays). Do NOT decref
  // children here (the collector does that through traverse), and don't
  // allocate. May be NULL.
  void (*finalize)(GCHeader *obj);

  // Optional: write a short human description for verifier reports.
  void (*describe)(GCHeader *obj, char *buf, size_t len);
} GCKind;

// Registers a kind and returns its id. Call during startup, before gc_new.
uint8_t gc_register_kind(const GCKind *kind);

// ---------------------------------------------------------------------------
// Allocation and counting
// ---------------------------------------------------------------------------

// Zeroed object of `size` bytes (including the header). It starts with a
// refcount of 0, which is fine: until a safe point, C locals keep it alive.
void *gc_new(uint8_t kind, size_t size);

#ifdef ARGON_GC_CUSTOM

void gc_zero_count(GCHeader *h); // refcount reached 0: list it in the ZCT
void gc_bad_decref(GCHeader *h); // decref of an object whose count is 0

static inline void gc_incref(void *obj) {
  GCHeader *h = obj;
  if (h && (h->flags & GC_MORTAL))
    h->refcount++;
}

static inline void gc_decref(void *obj) {
  GCHeader *h = obj;
  if (h && (h->flags & GC_MORTAL)) {
    if (__builtin_expect(h->refcount == 0, 0))
      gc_bad_decref(h);
    else if (--h->refcount == 0)
      gc_zero_count(h);
  }
}

#else // Boehm: nothing to count

static inline void gc_incref(void *obj) { (void)obj; }
static inline void gc_decref(void *obj) { (void)obj; }

#endif

// The common case: store `value` into a counted holder, replacing whatever
// was there. Increfs first, so assigning a slot to itself is safe.
//   gc_assign(&obj->field, value);
// (A macro using typeof so the slot keeps its real pointer type; casting to
// void ** would break strict aliasing.)
#define gc_assign(slot_ptr, value)                                             \
  do {                                                                         \
    __typeof__(slot_ptr) gc_slot_ = (slot_ptr);                                \
    __typeof__(*gc_slot_) gc_old_ = *gc_slot_;                                 \
    __typeof__(*gc_slot_) gc_new_ = (value);                                   \
    gc_incref(gc_new_);                                                        \
    *gc_slot_ = gc_new_;                                                       \
    gc_decref(gc_old_);                                                        \
  } while (0)

// incref every element of an array of object pointers (after copying a block
// of them into a counted holder with memcpy, say).
#define gc_incref_each(items, count)                                           \
  do {                                                                         \
    __typeof__(&(items)[0]) gc_items_ = (items);                               \
    for (size_t gc_i_ = 0, gc_n_ = (count); gc_i_ < gc_n_; gc_i_++)            \
      gc_incref(gc_items_[gc_i_]);                                             \
  } while (0)

// Make an object permanent: counting it does nothing and it is never freed.
void gc_immortalize(void *obj);
// Make every object allocated so far immortal (call at the end of bootstrap).
void gc_immortalize_all(void);

// A managed object that is NOT on the GC heap (a static) but may point at
// mortal objects: register it so the verifier can see its references.
void gc_register_static(void *obj);

// ---------------------------------------------------------------------------
// Epochs and safe points (see README: "Birth numbers")
// ---------------------------------------------------------------------------

typedef uint64_t gc_epoch;

// Call at the start of every runtime() invocation; pass the result to
// gc_leave when it returns. Inside, only objects born after gc_enter may be
// freed at safe points.
gc_epoch gc_enter(void);
void gc_leave(gc_epoch previous);

#ifdef ARGON_GC_CUSTOM

extern bool gc_work_due; // set when the ZCT is full or a collection is due
void gc_safe_point_slow(void);

// Call only where no C local holds an uncounted pointer that must survive.
// In the interpreter: the top of the bytecode loop, between instructions.
static inline void gc_safe_point(void) {
  if (__builtin_expect(gc_work_due, 0))
    gc_safe_point_slow();
}

#else

static inline void gc_safe_point(void) {}

#endif

// Run a full safe point now: free the ZCT, then collect cycles.
void gc_collect_now(void);

// ---------------------------------------------------------------------------
// Roots
// ---------------------------------------------------------------------------

// A root tracer calls visit(obj, ctx) for every object some non-heap holder
// points at. There are two sorts of holder:
typedef void (*gc_root_tracer)(gc_visit_fn visit, void *ctx);

// UNCOUNTED roots: holders that never incref, because they change far too
// often to be worth counting - the VM's registers, frames and argument lists.
// Every safe point scans them first, and anything they point at is kept alive
// (not freed from the ZCT, and treated as externally referenced by the cycle
// collector). This is the "deferred" in deferred reference counting.
void gc_add_uncounted_roots(gc_root_tracer tracer);

// COUNTED roots: holders that do incref but aren't managed objects (a global
// hashmap, say). The collector doesn't need them (their counts already keep
// things alive) but the verifier does, or their references look like leaks.
void gc_add_counted_roots(gc_root_tracer tracer);

typedef struct {
  size_t too_low;  // refcount < references found: would be freed while used
  size_t too_high; // refcount > references found: would leak forever
  size_t dangling; // a live object references a freed (zombie) object
  size_t invalid;  // a traverse function visited something that isn't a
                   // managed object at all (a traverse or flags bug)
} GCVerifyResult;

// Recount every reference and compare with every refcount; prints a report
// to stderr when anything is wrong. Meant for ARGON_GC_VERIFY and tests.
GCVerifyResult gc_verify(void);

// ---------------------------------------------------------------------------
// Statistics and setup
// ---------------------------------------------------------------------------

typedef struct {
  size_t allocated;     // gc_new calls
  size_t freed_by_rc;   // freed because their refcount reached zero
  size_t freed_by_cycle; // freed by the cycle collector
  size_t live;          // currently allocated (including zombies)
  size_t cycle_collections;
  double cycle_pause_ms_total, cycle_pause_ms_max;
} GCStats;

GCStats gc_stats(void);

// Reads ARGON_GC_STATS, ARGON_GC_STRESS, ARGON_GC_VERIFY, ARGON_GC_NOFREE.
void gc_init(void);
// Call when a second thread starts using the heap (stage 6 will replace this:
// for now it only makes allocation thread-safe).
void gc_enable_thread_safety(void);
void gc_shutdown(void); // prints the stats summary if ARGON_GC_STATS is set

#endif // ARGON_GC_H
