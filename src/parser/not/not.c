/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "not.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include <stdio.h>

ParsedValueReturn parse_not(char *file, DArray *tokens, size_t *index) {
  bool invert = true;
  (*index)++;
  while (tokens->size > *index) {
    Token *token = darray_get(tokens, *index);
    if (token->type != TOKEN_EXCLAMATION) {
      ParsedValueReturn value =
          parse_token_full(file, tokens, index, true, false);
      if (value.err.exists) {
        return value;
      } else if (!value.value) {
        return (ParsedValueReturn){create_err(token->line, token->column,
                                              token->length, file,
                                              "Syntax Error", "expected value"),
                                   NULL};
      }

      ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
      ParsedToBool *parsedToBool = checked_malloc(sizeof(ParsedToBool));
      parsedToBool->value = value.value;
      parsedToBool->invert = invert;
      parsedValue->data = parsedToBool;
      parsedValue->type = AST_TO_BOOL;
      return (ParsedValueReturn){no_err, parsedValue};
    }
    invert = !invert;
    (*index)++;
  }
  ArErr err = error_if_finished(file, tokens, index);
  return (ParsedValueReturn){err, NULL};
}

void free_not(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedToBool *parsedToBool = parsedValue->data;
  free_parsed(parsedToBool->value);
  free(parsedToBool->value);
  free(parsedToBool);
}