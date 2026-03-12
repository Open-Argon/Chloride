/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ASSIGN_H
#define ASSIGN_H
#include "../../parser.h"
#include "../../../lexer/token.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  ParsedValue * to;
  ArTokenType type;
  ParsedValue * from;
  uint64_t line;
  uint64_t column;
  size_t length;
} ParsedAssign;

ParsedValueReturn parse_assign(char *file, DArray *tokens, ParsedValue *assign_to,
                          size_t *index);

void free_parse_assign(void*ptr);

#endif // ASSIGN_H