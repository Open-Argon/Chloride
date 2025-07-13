/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "operations.h"
#include "../../memory.h"
#include "../parser.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValue convert_to_operation(DArray *to_operate_on, DArray *operations) {
  if (to_operate_on->size == 1) {
    return *((ParsedValue *)darray_get(to_operate_on, 0));
  }
  TokenType operation = 0;
  DArray positions;
  for (size_t i = 0; i < operations->size; i++) {
    TokenType *current_operation = darray_get(operations, i);
    if (operation < *current_operation) {
      if (operation != 0) {
        darray_free(&positions, NULL);
      }
      operation = *current_operation;
      darray_init(&positions, sizeof(size_t));
    }
    darray_push(&positions, &i);
  }
  ParsedValue parsedValue;
  parsedValue.type = AST_OPERATION;
  ParsedOperation *operationStruct = checked_malloc(sizeof(ParsedOperation));
  parsedValue.data = operationStruct;
  operationStruct->operation = operation;
  darray_init(&operationStruct->to_operate_on, sizeof(ParsedValue));
  size_t last_position = 0;
  size_t to_operate_on_last_position = 0;
  for (size_t i = 0; i < positions.size; i++) {
    size_t *position = darray_get(&positions, i);
    DArray to_operate_on_slice = darray_slice(
        to_operate_on, to_operate_on_last_position, (*position) + 1);
    DArray operations_slice =
        darray_slice(operations, last_position, *position);
    ParsedValue result =
        convert_to_operation(&to_operate_on_slice, &operations_slice);
    darray_push(&operationStruct->to_operate_on, &result);
    last_position = (*position);
    to_operate_on_last_position = (*position) + 1;
  }
  DArray to_operate_on_slice = darray_slice(
      to_operate_on, to_operate_on_last_position, to_operate_on->size);
  DArray operations_slice =
      darray_slice(operations, last_position, operations->size);
  ParsedValue result =
      convert_to_operation(&to_operate_on_slice, &operations_slice);
  darray_push(&operationStruct->to_operate_on, &result);
  darray_free(&positions, NULL);
  return parsedValue;
}

ParsedValueReturn parse_operations(char *file, DArray *tokens, size_t *index,
                                   ParsedValue *first_parsed_value) {
  DArray to_operate_on;
  darray_init(&to_operate_on, sizeof(ParsedValue));
  darray_push(&to_operate_on, first_parsed_value);
  free(first_parsed_value);

  DArray operations;
  darray_init(&operations, sizeof(TokenType));

  while (tokens->size > *index) {
    bool to_break = false;
    Token *token = darray_get(tokens, *index);
    switch (token->type) {
      SWITCH_OPERATIONS
      break;
    default:
      to_break = true;
    }
    if (to_break)
      break;
    darray_push(&operations, &token->type);
    (*index)++;
    ArErr err = error_if_finished(file, tokens, index);
    if (err.exists) {
      darray_free(&to_operate_on, free_parsed);
      darray_free(&operations, NULL);
      return (ParsedValueReturn){err, NULL};
    }
    ParsedValueReturn parsedValue =
        parse_token_full(file, tokens, index, true, false);
    if (parsedValue.err.exists) {
      darray_free(&to_operate_on, free_parsed);
      darray_free(&operations, NULL);
      return parsedValue;
    } else if (!parsedValue.value) {
      darray_free(&to_operate_on, free_parsed);
      darray_free(&operations, NULL);
      return (ParsedValueReturn){create_err(token->line, token->column,
                                            token->length, file, "Syntax Error",
                                            "expected value"),
                                 NULL};
    }
    darray_push(&to_operate_on, parsedValue.value);
    free(parsedValue.value);
  }
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedValue output = convert_to_operation(&to_operate_on, &operations);
  memcpy(parsedValue, &output, sizeof(ParsedValue));
  darray_free(&to_operate_on, NULL);
  darray_free(&operations, NULL);
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_operation(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedOperation *parsed_operation = parsedValue->data;
  darray_free(&parsed_operation->to_operate_on, free_parsed);
  free(parsed_operation);
}