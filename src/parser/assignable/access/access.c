/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "access.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../parser.h"
#include "../../string/string.h"
#include "../../../err.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_access(char *file, DArray *tokens, size_t *index,
                               ParsedValue *to_access) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedAccess *parsedAccess = checked_malloc(sizeof(ParsedAccess));
  parsedAccess->to_access = to_access;
  parsedAccess->access = NULL;
  parsedValue->type = AST_ACCESS;
  parsedValue->data = parsedAccess;
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){err, NULL};
  }
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_IDENTIFIER) {
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected identifier after dot"),
                               NULL};
  }
  parsedAccess->line = token->line;
  parsedAccess->column = token->column;
  parsedAccess->length = token->length;
  ParsedValueReturn parsedString = parse_string(token, false);
  if (parsedString.err.exists) {
    free_parsed(parsedValue);
    free(parsedValue);
    return parsedString;
  }
  parsedAccess->access = parsedString.value;
  (*index)++;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parse_access(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedAccess *parsedAccess = parsedValue->data;
  free_parsed(parsedAccess->to_access);
  free(parsedAccess->to_access);
  if (parsedAccess->access) {
    free_parsed(parsedAccess->access);
    free(parsedAccess->access);
  }
  free(parsedAccess);
}