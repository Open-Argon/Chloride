/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "operations.h"
#include "../../err.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../parser.h"
#include "../range/range.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Binary operator precedence, lowest to highest. Every level is
 * left-associative except exponentiation, which is right-associative.
 */
int operation_precedence(ArTokenType type) {
  switch (type) {
  case TOKEN_OR:
    return PRECEDENCE_OR;
  case TOKEN_AND:
    return PRECEDENCE_AND;
  case TOKEN_EQ:
  case TOKEN_NE:
    return PRECEDENCE_EQUALITY;
  case TOKEN_LT:
  case TOKEN_GT:
  case TOKEN_LE:
  case TOKEN_GE:
  case TOKEN_IN:
  case TOKEN_NOT_IN:
    return PRECEDENCE_COMPARISON;
  case TOKEN_TO:
  case TOKEN_UNTIL:
    return PRECEDENCE_RANGE;
  case TOKEN_PLUS:
  case TOKEN_MINUS:
    return PRECEDENCE_ADDITIVE;
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_FLOORDIV:
  case TOKEN_MODULO:
    return PRECEDENCE_MULTIPLICATIVE;
  case TOKEN_CARET:
    return PRECEDENCE_EXPONENT;
  default:
    return 0;
  }
}

static ParsedValue make_range(ParsedValue left, ParsedValue right,
                              Token *operation) {
  ParsedRange *range = checked_malloc(sizeof(ParsedRange));
  range->start = checked_malloc(sizeof(ParsedValue));
  *range->start = left;
  range->stop = checked_malloc(sizeof(ParsedValue));
  *range->stop = right;
  range->inclusive = operation->type == TOKEN_TO;
  range->line = operation->line;
  range->column = operation->column;
  range->length = operation->length;
  return (ParsedValue){AST_RANGE, range};
}

static ParsedValue make_operation(ParsedValue left, ParsedValue right,
                                  Token *operation, bool merge_left) {
  /*
   * Chains of the same left-associative operator (a + b + c) are kept as
   * a single n-ary operation rather than nested binary ones, which keeps
   * the number of registers the translator needs constant.
   */
  if (merge_left && left.type == AST_OPERATION) {
    ParsedOperation *existing = left.data;
    if (existing->operation == operation->type) {
      darray_push(&existing->to_operate_on, &right);
      return left;
    }
  }
  ParsedOperation *operationStruct = checked_malloc(sizeof(ParsedOperation));
  operationStruct->operation = operation->type;
  operationStruct->line = operation->line;
  operationStruct->column = operation->column;
  operationStruct->length = operation->length;
  darray_init(&operationStruct->to_operate_on, sizeof(ParsedValue));
  darray_push(&operationStruct->to_operate_on, &left);
  darray_push(&operationStruct->to_operate_on, &right);
  return (ParsedValue){AST_OPERATION, operationStruct};
}

/*
 * Builds the tree for operands[lo..hi] joined by operations[lo..hi-1] by
 * splitting on the lowest-precedence operator: the last one for
 * left-associative levels, the first one for right-associative levels.
 */
static ParsedValue build_operation(ParsedValue *operands, Token *operations,
                                   size_t lo, size_t hi) {
  if (lo == hi)
    return operands[lo];

  int lowest = 0;
  for (size_t i = lo; i < hi; i++) {
    int precedence = operation_precedence(operations[i].type);
    if (lowest == 0 || precedence < lowest)
      lowest = precedence;
  }

  bool right_associative = lowest == PRECEDENCE_EXPONENT;
  size_t split = lo;
  for (size_t i = lo; i < hi; i++) {
    if (operation_precedence(operations[i].type) == lowest) {
      split = i;
      if (right_associative)
        break;
    }
  }

  ParsedValue left = build_operation(operands, operations, lo, split);
  ParsedValue right = build_operation(operands, operations, split + 1, hi);
  Token *operation = &operations[split];

  if (operation->type == TOKEN_TO || operation->type == TOKEN_UNTIL)
    return make_range(left, right, operation);
  return make_operation(left, right, operation, !right_associative);
}

ParsedValueReturn parse_operations_with_minimum(char *file, DArray *tokens,
                                                size_t *index,
                                                ParsedValue *first_parsed_value,
                                                int minimum_precedence) {
  DArray to_operate_on;
  darray_init(&to_operate_on, sizeof(ParsedValue));
  darray_push(&to_operate_on, first_parsed_value);
  free(first_parsed_value);

  DArray operations;
  darray_init(&operations, sizeof(Token));

  while (tokens->size > *index) {
    bool to_break = false;
    Token *token = darray_get(tokens, *index);
    switch (token->type) {
      SWITCH_OPERATIONS
      to_break = operation_precedence(token->type) < minimum_precedence;
      break;
    default:
      to_break = true;
    }
    if (to_break)
      break;
    darray_push(&operations, token);
    (*index)++;
    skip_newlines_and_indents(tokens, index);
    ArErr err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
      darray_free(&to_operate_on, (void (*)(void *))free_parsed);
      darray_free(&operations, NULL);
      return (ParsedValueReturn){err, NULL};
    }
    ParsedValueReturn parsedValue =
        parse_token_full(file, tokens, index, true, false);
    if (is_error(&parsedValue.err)) {
      darray_free(&to_operate_on, (void (*)(void *))free_parsed);
      darray_free(&operations, NULL);
      return parsedValue;
    } else if (!parsedValue.value) {
      darray_free(&to_operate_on, (void (*)(void *))free_parsed);
      darray_free(&operations, NULL);
      return (ParsedValueReturn){
          path_specific_create_err(token->line, token->column, token->length,
                                   file, SyntaxError, "expected value"),
          NULL};
    }
    darray_push(&to_operate_on, parsedValue.value);
    free(parsedValue.value);
  }

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  *parsedValue = build_operation((ParsedValue *)to_operate_on.data,
                                 (Token *)operations.data, 0,
                                 to_operate_on.size - 1);
  darray_free(&to_operate_on, NULL);
  darray_free(&operations, NULL);
  return (ParsedValueReturn){no_err, parsedValue};
}

ParsedValueReturn parse_operations(char *file, DArray *tokens, size_t *index,
                                   ParsedValue *first_parsed_value) {
  return parse_operations_with_minimum(file, tokens, index,
                                       first_parsed_value, 0);
}

void free_operation(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedOperation *parsed_operation = parsedValue->data;
  darray_free(&parsed_operation->to_operate_on, (void (*)(void *))free_parsed);
  free(parsed_operation);
}
