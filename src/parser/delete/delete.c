/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "delete.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"

ParsedValueReturn parse_delete(char *file, DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  ParsedValueReturn value = parse_token(file, tokens, index, true);
  if (is_error(&value.err)) {
    return value;
  }
  if (!value.value) {
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected value"),
        NULL};
  }

  switch (value.value->type) {
  case AST_IDENTIFIER:
  case AST_ACCESS:
  case AST_ITEM_ACCESS:
    break;
  default:;
    free_parsed(value.value);
    free(value.value);
    return (ParsedValueReturn){
        path_specific_create_err(
            token->line, token->column, token->length, file, TypeError,
            "deleting something which can't be deleted"),
        NULL};
  }
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedDelete *parsedDelete = checked_malloc(sizeof(ParsedDelete));
  parsedValue->type = AST_DELETE;
  parsedValue->data = parsedDelete;
  parsedDelete->line = token->line;
  parsedDelete->column = token->column;
  parsedDelete->length = token->length;
  parsedDelete->value = value.value;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parsed_delete(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedDelete *parsed_delete = parsedValue->data;
  free_parsed(parsed_delete->value);
  free(parsed_delete->value);
  free(parsed_delete);
}