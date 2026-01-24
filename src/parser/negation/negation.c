/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "negation.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../err.h"
#include "../parser.h"

ParsedValueReturn parse_negation(char *file, DArray *tokens, size_t *index) {
  (*index)++;
  Token *token = darray_get(tokens, *index);
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }
  ParsedValueReturn value = parse_token_full(file, tokens, index, true, false);
  if (value.err.exists) {
    return value;
  } else if (!value.value) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected value"),
                               NULL};
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