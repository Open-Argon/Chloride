/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Shared between gc.c and verify.c. Not for use outside src/gc/.

#ifndef ARGON_GC_INTERNAL_H
#define ARGON_GC_INTERNAL_H

#include "gc.h"

typedef struct {
  bool print_stats;    // ARGON_GC_STATS
  bool verify;         // ARGON_GC_VERIFY
  bool nofree;         // ARGON_GC_NOFREE
  size_t stress_every; // ARGON_GC_STRESS
} GCConfig;

extern GCConfig gc_config;
extern const GCKind *gc_kinds[256];
extern size_t gc_kind_count;

extern GCHeader **gc_statics;
extern size_t gc_static_count;
extern gc_root_tracer *gc_root_tracers;
extern size_t gc_root_tracer_count;

static inline const GCKind *gc_kind_of(const GCHeader *h) {
  return gc_kinds[h->kind];
}

// Human-readable "Kind@0x1234 (description)" for reports.
void gc_describe(GCHeader *h, char *buf, size_t len);

// A growable array of headers, allocated with malloc (scratch space for a
// collection; never holds the only reference to anything).
typedef struct {
  GCHeader **items;
  size_t len, cap;
} GCVec;

void gc_vec_push(GCVec *v, GCHeader *h);
void gc_vec_free(GCVec *v);

#endif // ARGON_GC_INTERNAL_H
