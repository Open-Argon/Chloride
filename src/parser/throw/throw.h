/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef THROW_H
#define THROW_H

#include "../parser.h"

typedef struct {
  ParsedValue * value;
  int64_t line;
  int64_t column;
  int64_t length;
} ParsedThrow;

ParsedValueReturn parse_throw(char *file, DArray *tokens, size_t *index);

void free_parsed_throw(void *ptr);

#endif // THROW_H