/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ITEM_ACCESS_H
#define ITEM_ACCESS_H
#include "../../parser.h"
#include "../../../lexer/token.h"  // for Token
typedef struct {
  ParsedValue *to_access;
  ParsedValue **items;
  size_t itemc;
  size_t line;
  size_t column;
  size_t length;
} ParsedItemAccess;

ParsedValueReturn parse_item_access(char *file, DArray *tokens, size_t *index,
                               ParsedValue *to_access);

void free_parse_item_access(void *ptr);

#endif // ACCESS_H