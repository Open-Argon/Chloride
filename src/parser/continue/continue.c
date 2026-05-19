/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "continue.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../err.h"

ParsedValueReturn parse_continue(DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedContinueOrBreak *parsedContinue = checked_malloc(sizeof(ParsedContinueOrBreak));
  parsedValue->type = AST_CONTINUE;
  parsedValue->data = parsedContinue;
  parsedContinue->line = token->line;
  parsedContinue->column = token->column;
  parsedContinue->length = token->length;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parsed_continue(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedContinueOrBreak *parsedContinue = parsedValue->data;
  free(parsedContinue);
}