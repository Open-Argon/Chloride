/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RETURN_TYPES_H
#define RETURN_TYPES_H
#include <stdint.h>
#include "arobject.h"

#define ERR_MSG_MAX_LEN 256

typedef struct ArErr {
  bool exists;
  char *path;
  int64_t line;
  int64_t column;
  int length;
  char type[32];
  char message[ERR_MSG_MAX_LEN];
} ArErr;

typedef struct Execution {
  ArErr err;
  Stack stack;
} Execution;
#endif // RETURN_TYPES_