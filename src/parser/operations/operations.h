/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPERATIONS_H
#define OPERATIONS_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

typedef struct {
  ArTokenType operation;
  DArray to_operate_on; // ParsedValue[]
} ParsedOperation;

ParsedValueReturn parse_operations(char *file, DArray *tokens, size_t *index,
                              ParsedValue *first_parsed_value);

void free_operation(void *ptr);

#endif // OPERATIONS_H