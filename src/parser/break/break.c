/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "break.h"
#include "../continue/continue.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../err.h"

ParsedValueReturn parse_break(DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedContinueOrBreak *parsedBreak = checked_malloc(sizeof(ParsedContinueOrBreak));
  parsedValue->type = AST_BREAK;
  parsedValue->data = parsedBreak;
  parsedBreak->line = token->line;
  parsedBreak->column = token->column;
  parsedBreak->length = token->length;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parsed_break(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedContinueOrBreak *parsedContinue = parsedValue->data;
  free(parsedContinue);
}