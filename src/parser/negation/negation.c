/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "negation.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../err.h"
#include "../parser.h"
#include "../operations/operations.h"
#include "../../runtime/objects/exceptions/exceptions.h"

ParsedValueReturn parse_negation(char *file, DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    return (ParsedValueReturn){err, NULL};
  }
  ParsedValueReturn value = parse_token_full(file, tokens, index, true, false);
  if (is_error(&value.err)) {
    return value;
  } else if (!value.value) {
    return (ParsedValueReturn){path_specific_create_err(token->line, token->column,
                                          token->length, file, SyntaxError,
                                          "expected value"),
                               NULL};
  }

  // Exponentiation binds more tightly than negation: -2 ^ 2 is -(2 ^ 2).
  size_t before_newlines = *index;
  skip_newlines_and_indents(tokens, index);
  if (*index < tokens->size &&
      ((Token *)darray_get(tokens, *index))->type == TOKEN_CARET) {
    value = parse_operations_with_minimum(file, tokens, index, value.value,
                                          PRECEDENCE_EXPONENT);
    if (is_error(&value.err))
      return value;
  } else {
    *index = before_newlines;
  }
  if (value.value->type == AST_NUMBER) {
    mpq_t *number = value.value->data;
    mpq_neg(*number, *number);
    return value;
  }
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->data = value.value;
  parsedValue->type = AST_NEGATION;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_negation(void *ptr) {
  ParsedValue *parsedValue = ptr;
  free_parsed(parsedValue->data);
  free(parsedValue->data);
}