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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_access(char *file, DArray *tokens, size_t *index,
                               ParsedValue *to_access) {
  Token *first_token = darray_get(tokens, *index);
  (*index)++;
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedAccess *parsedAccess = checked_malloc(sizeof(ParsedAccess));
  parsedAccess->to_access = *to_access;
  parsedValue->type = AST_ACCESS;
  parsedValue->data = parsedAccess;
  free(to_access);
  darray_init(&parsedAccess->access, sizeof(ParsedValue));
  if (first_token->type == TOKEN_DOT) {
    ArErr err = error_if_finished(file, tokens, index);
    if (err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }
    Token *token = darray_get(tokens, *index);
    ParsedValueReturn parsedString = parse_string(token, false);
    if (parsedString.err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return parsedString;
    }
    darray_push(&parsedAccess->access, parsedString.value);
    free(parsedString.value);
    parsedAccess->access_fields = true;
  } else {
    parsedAccess->access_fields = false;
    Token *token = first_token;
    while (true) {
      skip_newlines_and_indents(tokens, index);
      ArErr err = error_if_finished(file, tokens, index);
      if (err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }
      ParsedValueReturn parsedAccessValue =
          parse_token(file, tokens, index, true);
      if (parsedAccessValue.err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return parsedAccessValue;
      } else if (!parsedAccessValue.value) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){create_err(token->line, token->column,
                                              token->length, file,
                                              "Syntax Error", "expected value"),
                                   NULL};
      }
      darray_push(&parsedAccess->access, parsedAccessValue.value);
      free(parsedAccessValue.value);
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }
      token = darray_get(tokens, *index);
      if (token->type == TOKEN_RBRACKET) {
        break;
      } else if (token->type != TOKEN_COLON) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){create_err(token->line, token->column,
                                              token->length, file,
                                              "Syntax Error", "expected colon"),
                                   NULL};
      }
      (*index)++;
    }
  }
  (*index)++;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parse_access(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedAccess *parsedAccess = parsedValue->data;
  free_parsed(&parsedAccess->to_access);
  darray_free(&parsedAccess->access, free_parsed);
  free(parsedAccess);
}