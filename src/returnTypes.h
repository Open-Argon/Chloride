/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RETURN_TYPES_H
#define RETURN_TYPES_H
#include <stdint.h>
#include "arobject.h"

#define ERR_MSG_MAX_LEN 32

typedef struct ArErr {
  char *path;
  int64_t line;
  int64_t column;
  int length;
  char message[ERR_MSG_MAX_LEN];
  char type[16];
  bool exists;
} ArErr;
#endif // RETURN_TYPES_