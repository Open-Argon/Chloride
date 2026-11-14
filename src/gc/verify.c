/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// The refcount verifier (ARGON_GC_VERIFY=1, or gc_verify() directly).
//
// Trial deletion trusts every refcount, so a single wrong count means either
// a use-after-free or a permanent leak. The verifier finds them by doing the
// counting from scratch:
//
//   1. zero a tally for every live mortal object
//   2. visit every reference held by every live object (mortal or immortal),
//      every registered static, and every root tracer (VM registers, frames...)
//      adding 1 to the tally of each object referenced
//   3. compare each tally with the object's refcount
//
// refcount < tally   too LOW:  something stored a pointer without incref.
//                    The object will be freed while still referenced.
// refcount > tally   too HIGH: something dropped a pointer without decref,
//                    or a counted holder isn't visible to the verifier
//                    (missing traverse or root tracer). It will never be freed.
// tally from zombie  DANGLING (only with ARGON_GC_NOFREE): a live object
//                    still points at something that has already been freed.
// not an object      INVALID: a traverse function visited a pointer that is
//                    neither a live heap object nor a registered static, so
//                    the traverse (or a hashmap's gc_flags) is wrong.
//
// The tally reuses GCHeader.gc_refs, which is free outside cycle collection.

#ifdef ARGON_GC_CUSTOM

#include "gc_internal.h"
#include "heap.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_EXAMPLES 10

typedef struct {
  GCHeader *holder; // whose references we're counting (NULL = a root)
  GCVerifyResult result;
  GCHeader **statics; // sorted copy of gc_statics, for lookups
} VerifyCtx;

static int compare_ptr(const void *a, const void *b) {
  uintptr_t x = (uintptr_t)*(GCHeader *const *)a;
  uintptr_t y = (uintptr_t)*(GCHeader *const *)b;
  return x < y ? -1 : x > y;
}

// Is `p` the start of a live heap object or a registered static? Must not
// dereference p until this says yes.
static bool is_managed_object(VerifyCtx *v, GCHeader *p) {
  if (heap_cell_of(p) == p)
    return p->kind < gc_kind_count;
  return v->statics && bsearch(&p, v->statics, gc_static_count,
                               sizeof(GCHeader *), compare_ptr) != NULL;
}

static void reset_tally(void *cell, void *ctx) {
  (void)ctx;
  ((GCHeader *)cell)->gc_refs = 0;
}

static void count_reference(GCHeader *child, void *ctx) {
  VerifyCtx *v = ctx;
  if (!child)
    return;
  if (!is_managed_object(v, child)) {
    if (v->result.invalid++ < MAX_EXAMPLES) {
      char from[160] = "a root";
      if (v->holder)
        gc_describe(v->holder, from, sizeof from);
      fprintf(stderr,
              "gc verify: INVALID %s holds %p, which is not a managed object "
              "(wrong traverse or hashmap gc_flags)\n",
              from, (void *)child);
    }
    return;
  }
  if (!(child->flags & GC_MORTAL))
    return; // immortal objects aren't counted
  if (child->flags & GC_ZOMBIE) {
    if (v->result.dangling++ < MAX_EXAMPLES) {
      char from[160] = "a root", to[160];
      if (v->holder)
        gc_describe(v->holder, from, sizeof from);
      gc_describe(child, to, sizeof to);
      fprintf(stderr, "gc verify: DANGLING %s -> %s (already freed)\n", from,
              to);
    }
    return;
  }
  child->gc_refs++;
}

static void check_uncounted(GCHeader *child, void *ctx) {
  VerifyCtx *v = ctx;
  if (!child)
    return;
  uint32_t saved = 0;
  bool managed = is_managed_object(v, child);
  if (managed)
    saved = child->gc_refs;
  count_reference(child, ctx); // reports invalid / dangling
  if (managed)
    child->gc_refs = saved;    // ... but doesn't count
}

static void count_from_object(void *cell, void *ctx) {
  GCHeader *h = cell;
  VerifyCtx *v = ctx;
  const GCKind *k = gc_kind_of(h);
  // A zombie's references were released when it was "freed".
  if ((h->flags & GC_ZOMBIE) || !k || !k->traverse)
    return;
  v->holder = h;
  k->traverse(h, count_reference, v);
}

static void compare(void *cell, void *ctx) {
  GCHeader *h = cell;
  VerifyCtx *v = ctx;
  if (!(h->flags & GC_MORTAL) || (h->flags & GC_ZOMBIE))
    return;
  if (h->refcount == h->gc_refs)
    return;
  bool low = h->refcount < h->gc_refs;
  size_t n = low ? v->result.too_low++ : v->result.too_high++;
  if (n < MAX_EXAMPLES) {
    char desc[160];
    gc_describe(h, desc, sizeof desc);
    fprintf(stderr,
            "gc verify: refcount too %s: %s has refcount %u but %u "
            "reference%s found (%s)\n",
            low ? "LOW" : "HIGH", desc, h->refcount, h->gc_refs,
            h->gc_refs == 1 ? " was" : "s were",
            low ? "missing incref; will be freed while in use"
                : "missing decref or untraced holder; will leak");
  }
}

GCVerifyResult gc_verify(void) {
  VerifyCtx v = {0};
  if (gc_static_count) {
    v.statics = malloc(gc_static_count * sizeof(GCHeader *));
    if (v.statics) {
      for (size_t i = 0; i < gc_static_count; i++)
        v.statics[i] = gc_statics[i];
      qsort(v.statics, gc_static_count, sizeof(GCHeader *), compare_ptr);
    }
  }
  heap_for_each(reset_tally, NULL);

  heap_for_each(count_from_object, &v);
  for (size_t i = 0; i < gc_static_count; i++)
    count_from_object(gc_statics[i], &v);
  v.holder = NULL;
  for (size_t i = 0; i < gc_counted_root_count; i++)
    gc_counted_roots[i](count_reference, &v);
  // Uncounted roots aren't in any refcount, so they don't add to the tallies,
  // but what they point at must still be real (and not freed) objects.
  for (size_t i = 0; i < gc_uncounted_root_count; i++)
    gc_uncounted_roots[i](check_uncounted, &v);

  heap_for_each(compare, &v);
  heap_for_each(reset_tally, NULL); // gc_refs must be 0 outside collections
  free(v.statics);

  GCVerifyResult r = v.result;
  if (r.too_low || r.too_high || r.dangling || r.invalid)
    fprintf(stderr,
            "gc verify: %zu too low, %zu too high, %zu dangling, %zu invalid%s\n",
            r.too_low, r.too_high, r.dangling, r.invalid,
            (r.too_low > MAX_EXAMPLES || r.too_high > MAX_EXAMPLES ||
             r.dangling > MAX_EXAMPLES || r.invalid > MAX_EXAMPLES)
                ? " (only the first few of each are listed)"
                : "");
  return v.result;
}

#endif // ARGON_GC_CUSTOM
