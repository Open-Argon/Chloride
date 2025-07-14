/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "return.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../literals/literals.h"

ParsedValueReturn parse_return(char *file, DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedReturn *parsedReturn = checked_malloc(sizeof(ParsedReturn));
  parsedValue->type = AST_RETURN;
  parsedValue->data = parsedReturn;
  parsedReturn->line = token->line;
  parsedReturn->column = token->column;
  parsedReturn->length = token->length;
  parsedReturn->value = parse_null();
  if (*index >= tokens->size)
    return (ParsedValueReturn){no_err, parsedValue};
  token = darray_get(tokens, *index);
  if (token->type == TOKEN_NEW_LINE)
    return (ParsedValueReturn){no_err, parsedValue};
  ParsedValueReturn value = parse_token(file, tokens, index, true);
  if (value.err.exists) {
    free_parsed(parsedValue);
    free(parsedValue);
    return value;
  }
  if (!value.value) {
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected value"),
                               NULL};
  }
  free_parsed(parsedReturn->value);
  free(parsedReturn->value);
  parsedReturn->value = value.value;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parsed_return(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedReturn *parsed_return = parsedValue->data;
  free_parsed(parsed_return->value);
  free(parsed_return->value);
  free(parsed_return);
}