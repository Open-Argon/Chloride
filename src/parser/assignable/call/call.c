/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "call.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../parser.h"
#include "../../../err.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_call(char *file, DArray *tokens, size_t *index,
                             ParsedValue *to_call) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedCall *call = checked_malloc(sizeof(ParsedCall));
  Token *token = darray_get(tokens, *index);
  call->line = token->line;
  call->column = token->column;
  call->to_call = to_call;
  parsedValue->data = call;
  parsedValue->type = AST_CALL;
  darray_init(&call->args, sizeof(ParsedValue));
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){err, NULL};
  }
  token = darray_get(tokens, *index);
  if (token->type != TOKEN_RPAREN) {
    while ((*index) < tokens->size) {
      skip_newlines_and_indents(tokens, index);
      ArErr err = error_if_finished(file, tokens, index);
      if (err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }
      ParsedValueReturn parsedArg = parse_token(file, tokens, index, true);
      if (parsedArg.err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return parsedArg;
      } else if (!parsedArg.value) {
        free_parsed(parsedValue);
        free(parsedValue);

        return (ParsedValueReturn){
            create_err(token->line, token->column, token->length, file,
                       "Syntax Error", "expected argument"),
            NULL};
      }
      darray_push(&call->args, parsedArg.value);
      free(parsedArg.value);
      err = error_if_finished(file, tokens, index);
      if (err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }
      token = darray_get(tokens, *index);
      if (token->type == TOKEN_RPAREN) {
        break;
      } else if (token->type != TOKEN_COMMA) {
        free_parsed(parsedValue);
        free(parsedValue);

        return (ParsedValueReturn){create_err(token->line, token->column,
                                              token->length, file,
                                              "Syntax Error", "expected comma"),
                                   NULL};
      }
      (*index)++;
      err = error_if_finished(file, tokens, index);
      if (err.exists) {
        free_parsed(parsedValue);
        free(parsedValue);
        return (ParsedValueReturn){err, NULL};
      }
    }
  }
  (*index)++;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parse_call(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedCall *parsedCall = parsedValue->data;

  darray_free(&parsedCall->args, (void (*)(void *))free_parsed);
  free_parsed(parsedCall->to_call);
  free(parsedCall->to_call);
  free(parsedCall);
}