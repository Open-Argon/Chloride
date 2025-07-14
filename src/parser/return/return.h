/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RETURN_H
#define RETURN_H

#include "../parser.h"

typedef struct {
  ParsedValue * value;
  int64_t line;
  int64_t column;
  int64_t length;
} ParsedReturn;

ParsedValueReturn parse_return(char *file, DArray *tokens, size_t *index);

void free_parsed_return(void *ptr);

#endif // RETURN_H