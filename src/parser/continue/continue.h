/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CONTINUE_H
#define CONTINUE_H

#include "../parser.h"

typedef struct {
  int64_t line;
  int64_t column;
  int64_t length;
} ParsedContinue;

ParsedValueReturn parse_continue(DArray *tokens, size_t *index);

void free_parsed_continue(void *ptr);

#endif // CONTINUE_H