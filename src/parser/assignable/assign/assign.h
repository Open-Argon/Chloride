/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ASSIGN_H
#define ASSIGN_H
#include "../../parser.h"
#include "../../../lexer/token.h"

typedef struct {
  ParsedValue * to;
  TokenType type;
  ParsedValue * from;
} ParsedAssign;

ParsedValueReturn parse_assign(char *file, DArray *tokens, ParsedValue *assign_to,
                          size_t *index);

void free_parse_assign(void*ptr);

#endif // ASSIGN_H