/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ITEM_ACCESS_H
#define ITEM_ACCESS_H
#include "../../parser.h"

typedef struct {
  DArray items;
} ParsedSubscript;

typedef struct {
  ParsedValue *to_access;
  DArray subscripts;
  size_t line;
  size_t column;
  size_t length;
} ParsedItemAccess;

void free_subscript_item(void *ptr);

ParsedValueReturn parse_item_access(char *file, DArray *tokens, size_t *index,
                               ParsedValue *to_access);

void free_parse_item_access(void *ptr);

#endif // ACCESS_H