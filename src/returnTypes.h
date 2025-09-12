/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RETURN_TYPES_H
#define RETURN_TYPES_H
#include <stdint.h>
#include "arobject.h"
#include <stdio.h>


typedef struct ArErr {
  char path[FILENAME_MAX];
  char message[128];
  char type[64];
  int64_t line;
  int64_t column;
  int length;
  bool exists;
} ArErr;
#endif // RETURN_TYPES_