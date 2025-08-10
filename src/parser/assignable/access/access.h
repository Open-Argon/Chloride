/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ACCESS_H
#define ACCESS_H
#include "../../parser.h"
#include "../../../lexer/token.h"  // for Token

typedef struct {
  ParsedValue to_access;
  bool access_fields;
  DArray access;
  size_t line;
  size_t column;
  size_t length;
} ParsedAccess;

// Function declaration for parsing an identifier
ParsedValueReturn parse_access(char *file, DArray *tokens, size_t *index,
                          ParsedValue *to_access);

void free_parse_access(void *ptr);

#endif // ACCESS_H