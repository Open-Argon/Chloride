/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RETURN_TYPES_H
#define RETURN_TYPES_H
#include <limits.h>
#include <stdint.h>
#include "arobject.h"
#include "runtime/internals/dynamic_array_armem/darray_armem.h"


typedef struct ArErr {
  char message[128];
  char type[64];
  bool exists;
  darray_armem *stack_trace;
} ArErr;
#endif // RETURN_TYPES_