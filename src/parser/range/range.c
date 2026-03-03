/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "range.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_range(char *file, DArray *tokens, size_t *index,
                              ParsedValue *start) {
  Token *token = darray_get(tokens, *index);
  bool inclusive = token->type == TOKEN_TO;
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }

  ParsedValueReturn stop = parse_token(file, tokens, index, true);
  if (stop.err.exists) {
    return stop;
  } else if (!stop.value) {
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, "Syntax Error", "expected value"),
        NULL};
  }

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedRange *range = checked_malloc(sizeof(ParsedRange));

  range->start = start;
  range->stop = stop.value;
  range->inclusive = inclusive;
  range->line = token->line;
  range->column = token->column;
  range->length = token->length;

  parsedValue->data = range;
  parsedValue->type = AST_RANGE;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parse_range(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedRange *parsedRange = parsedValue->data;

  free_parsed(parsedRange->start);
  free(parsedRange->start);
  free_parsed(parsedRange->stop);
  free(parsedRange->stop);
  free(parsedRange);
}