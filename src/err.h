/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ERROR_H
#define ERROR_H
#include "arobject.h"
#include <limits.h>
#include <stdio.h>

ArErr create_err(ArgonObject *type, const char *fmt, ...);
ArErr path_specific_create_err(int64_t line, int64_t column, int64_t length,
                               char *path, ArgonObject *type,const char *fmt,
                               ...);
ArErr vcreate_err(ArgonObject *type, const char *fmt, va_list args);
void output_err(ArErr *err);
#endif // ERROR_H