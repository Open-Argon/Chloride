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
  size_t line;
  size_t column;
  size_t length;
} ParsedOperation;

// Binary operator precedence levels, lowest to highest.
enum {
  PRECEDENCE_OR = 1,
  PRECEDENCE_AND,
  PRECEDENCE_EQUALITY,       // == !=
  PRECEDENCE_COMPARISON,     // < > <= >= in not in
  PRECEDENCE_RANGE,          // to until
  PRECEDENCE_ADDITIVE,       // + -
  PRECEDENCE_MULTIPLICATIVE, // * / // %
  PRECEDENCE_EXPONENT,       // ^ (right-associative)
};

int operation_precedence(ArTokenType type);

ParsedValueReturn parse_operations(char *file, DArray *tokens, size_t *index,
                              ParsedValue *first_parsed_value);

// Like parse_operations, but stops before any operator whose precedence is
// lower than minimum_precedence.
ParsedValueReturn parse_operations_with_minimum(char *file, DArray *tokens,
                                                size_t *index,
                                                ParsedValue *first_parsed_value,
                                                int minimum_precedence);

void free_operation(void *ptr);

#endif // OPERATIONS_H