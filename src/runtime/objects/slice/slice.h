/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_SLICE_H
#define RUNTIME_SLICE_H
#include "../../../arobject.h"
#include "../object.h"

typedef struct {
  int64_t start;
  int64_t stop;
  int64_t step;
} SliceIndices;

extern ArgonObject *ARGON_SLICE_TYPE;

void init_slice_type();

int slice_indices(ArgonObject *self, int64_t length, SliceIndices *out,
                         ArErr *err, ArgonNativeAPI *api);

EXPOSE_ARGON_METHOD(ARGON_SLICE_TYPE, indices)

#endif // RUNTIME_SLICE_H