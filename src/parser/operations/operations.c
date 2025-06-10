#include "operations.h"
#include "../parser.h"
#include "../../memory.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValue *convert_to_operation(DArray * to_operate_on, DArray * operations) {
  if (to_operate_on->size == 1) {
    ParsedValue * parsedValue = malloc(sizeof(ParsedValue));
    memcpy(parsedValue, darray_get(to_operate_on, 0), sizeof(ParsedValue));
    return parsedValue;
  }
  TokenType operation = 0;
  DArray positions;
  for (size_t i = 0; i<operations->size;i++) {
    TokenType * current_operation = darray_get(operations, i);
    if (operation < *current_operation) {
      if (operation!=0)  {
        darray_free(&positions, NULL);
      }
      operation = *current_operation;
      darray_init(&positions, sizeof(size_t));
    }
    darray_push(&positions, &i);
  }
  size_t last_position = operations->size-1;
  darray_push(&positions, &last_position);
  ParsedValue * parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type  = AST_OPERATION;
  ParsedOperation * operationStruct = checked_malloc(sizeof(ParsedOperation));
  parsedValue->data = operationStruct;
  operationStruct->operation = operation;
  darray_init(&operationStruct->to_operate_on, sizeof(ParsedValue));
  last_position = 0;
  for (size_t i = 0; i<positions.size;i++) {
    size_t *position = darray_get(&positions, i);
    DArray to_operate_on_slice = darray_slice(to_operate_on, last_position, *position+1);
    DArray operations_slice = darray_slice(operations, last_position, *position);
    darray_push(&operationStruct->to_operate_on, convert_to_operation(&to_operate_on_slice, &operations_slice));
    last_position = *position;
  }
  darray_free(&positions, NULL);
  return parsedValue;
}

ParsedValue *parse_operations(char *file, DArray *tokens, size_t *index,
                              ParsedValue *first_parsed_value) {
  DArray to_operate_on;
  darray_init(&to_operate_on, sizeof(ParsedValue));
  darray_push(&to_operate_on, first_parsed_value);

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
    error_if_finished(file, tokens, index);
    darray_push(&to_operate_on, parse_token_full(file, tokens, index, true, false));
  }
  ParsedValue *output = convert_to_operation(&to_operate_on, &operations);
  darray_free(&to_operate_on, NULL);
  darray_free(&operations, NULL);
  return output;
}

void free_operation(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedOperation *parsed_operation = parsedValue->data;
  darray_free(&parsed_operation->to_operate_on, free_parsed);
  free(parsed_operation);
}