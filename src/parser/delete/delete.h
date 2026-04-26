/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DELETE_H
#define DELETE_H
#include "../parser.h"

typedef struct {
  ParsedValue * value;
  int64_t line;
  int64_t column;
  int64_t length;
} ParsedDelete;

ParsedValueReturn parse_delete(char *file, DArray *tokens, size_t *index);

void free_parsed_delete(void *ptr);

#endif // DELETE_H