/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RANGE_H
#define RANGE_H
#include "../parser.h"

typedef struct {
  ParsedValue * start;
  ParsedValue * stop;
  bool inclusive;
  uint64_t line;
  uint64_t column;
  uint64_t length;
} ParsedRange;

// Function declaration for parsing an identifier
ParsedValueReturn parse_range(char *file, DArray *tokens, size_t *index,
                             ParsedValue *start);
void free_parse_range(void *ptr);

#endif // RANGE_H