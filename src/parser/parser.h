/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PARSER_H
#define PARSER_H

#include "../dynamic_array/darray.h"
#include "../returnTypes.h"
#include <stdbool.h>
#include <stddef.h>

#define SWITCH_OPERATIONS                                                      \
  case TOKEN_AND:                                                              \
  case TOKEN_OR:                                                               \
  case TOKEN_NOT_IN:                                                           \
  case TOKEN_IN:                                                               \
  case TOKEN_LE:                                                               \
  case TOKEN_GE:                                                               \
  case TOKEN_LT:                                                               \
  case TOKEN_GT:                                                               \
  case TOKEN_NE:                                                               \
  case TOKEN_EQ:                                                               \
  case TOKEN_PLUS:                                                             \
  case TOKEN_MINUS:                                                            \
  case TOKEN_MODULO:                                                           \
  case TOKEN_STAR:                                                             \
  case TOKEN_FLOORDIV:                                                         \
  case TOKEN_SLASH:                                                            \
  case TOKEN_CARET:

typedef struct LinkedList LinkedList;

typedef enum {
  AST_STRING,
  AST_ASSIGN,
  AST_IDENTIFIER,
  AST_CLASS,
  AST_NUMBER,
  AST_IF,
  AST_ACCESS,
  AST_ITEM_ACCESS,
  AST_CALL,
  AST_DECLARATION,
  AST_NULL,
  AST_BOOLEAN,
  AST_DOWRAP,
  AST_OPERATION,
  AST_LIST,
  AST_DICTIONARY,
  AST_FUNCTION,
  AST_RETURN,
  AST_WHILE,
  AST_TO_BOOL,
  AST_NEGATION
} ValueType;

extern const char *ValueTypeNames[];

typedef struct {
  ValueType type;
  void *data;
} ParsedValue;

typedef struct {
  ArErr err;
  ParsedValue *value;
} ParsedValueReturn;

ArErr parser(char *file, DArray *parsed, DArray *tokens, bool inline_flag);

ParsedValueReturn parse_token_full(char *file, DArray *tokens, size_t *index,
                              bool inline_flag, bool process_operations);

ParsedValueReturn parse_token(char *file, DArray *tokens, size_t *index,
                         bool inline_flag);

void free_parsed(void *ptr);

__attribute__((warn_unused_result)) ArErr error_if_finished(char *file, DArray *tokens, size_t *index);

size_t skip_newlines_and_indents(DArray *tokens, size_t *index);

#endif // PARSER_H