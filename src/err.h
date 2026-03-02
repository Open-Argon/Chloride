/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RETURN_TYPE_H
#define RETURN_TYPE_H
#include "returnTypes.h"
#include <limits.h>
#include <stdio.h>

extern const ArErr no_err;

struct StackTraceFrame {
  uint64_t line;
  uint64_t column;
  uint64_t length;
  char *path;
};

ArErr create_err(const char *type, const char *fmt, ...);
ArErr path_specific_create_err(int64_t line, int64_t column, int64_t length,
                               char *path, const char *type, const char *fmt,
                               ...);
ArErr vcreate_err(const char *type, const char *fmt, va_list args);
void output_err(ArErr *err);
#endif // RETURN_TYPE_H