/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "throw.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"

ParsedValueReturn parse_throw(char *file, DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedThrow *parsedThrow = checked_malloc(sizeof(ParsedThrow));
  parsedValue->type = AST_THROW;
  parsedValue->data = parsedThrow;
  parsedThrow->line = token->line;
  parsedThrow->column = token->column;
  parsedThrow->length = token->length;
  parsedThrow->value = NULL;
  ParsedValueReturn value = parse_token(file, tokens, index, true);
  if (is_error(&value.err)) {
    free_parsed(parsedValue);
    free(parsedValue);
    return value;
  }
  if (!value.value) {
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected value"),
        NULL};
  }
  parsedThrow->value = value.value;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parsed_throw(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedThrow *parsedThrow = parsedValue->data;
  if (parsedThrow->value) {
    free_parsed(parsedThrow->value);
    free(parsedThrow->value);
  }
  free(parsedThrow);
}