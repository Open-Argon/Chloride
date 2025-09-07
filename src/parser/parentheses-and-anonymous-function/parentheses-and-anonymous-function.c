/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "parentheses-and-anonymous-function.h"
#include "../../memory.h"
#include "../assignable/identifier/identifier.h"
#include "../function/function.h"
#include <stddef.h>
#include <string.h>

ParsedValueReturn parse_parentheses(char *file, DArray *tokens, size_t *index) {
  (*index)++;
  skip_newlines_and_indents(tokens, index);
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    // darray_free(&list, free_parsed);
    return (ParsedValueReturn){err, NULL};
  }
  DArray list;
  darray_init(&list, sizeof(ParsedValue));
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_RPAREN) {
    while (*index < tokens->size) {
      ParsedValueReturn parsedItem = parse_token(file, tokens, index, true);
      if (parsedItem.err.exists) {
        darray_free(&list, free_parsed);
        return parsedItem;
      }
      darray_push(&list, parsedItem.value);
      free(parsedItem.value);
      skip_newlines_and_indents(tokens, index);
      ArErr err = error_if_finished(file, tokens, index);
      if (err.exists) {
        darray_free(&list, free_parsed);
        return (ParsedValueReturn){err, NULL};
      }
      token = darray_get(tokens, *index);
      if (token->type == TOKEN_RPAREN) {
        break;
      } else if (token->type != TOKEN_COMMA) {
        darray_free(&list, free_parsed);
        return (ParsedValueReturn){create_err(token->line, token->column,
                                              token->length, file,
                                              "Syntax Error", "expected comma"),
                                   NULL};
      }
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (err.exists) {
        darray_free(&list, free_parsed);
        return (ParsedValueReturn){err, NULL};
      }
    }
  }
  (*index)++;
  if (*index < tokens->size) {
    token = darray_get(tokens, *index);
    if (token->type == TOKEN_ASSIGN) {
      (*index)++;
      ArErr err = error_if_finished(file, tokens, index);
      if (err.exists) {
        darray_free(&list, free_parsed);
        return (ParsedValueReturn){err, NULL};
      }
      DArray parameters;
      darray_init(&parameters, sizeof(char *));
      for (size_t i = 0; i < list.size; i++) {
        ParsedValue *item = darray_get(&list, i);
        if (item->type != AST_IDENTIFIER) {
          darray_free(&list, free_parsed);
          darray_free(&parameters, free_parameter);
          return (ParsedValueReturn){
              create_err(token->line, token->column, token->length, file,
                         "Syntax Error", "expected identifier"),
              NULL};
        }
        char *param = strdup(((ParsedIdentifier *)item->data)->name);
        darray_push(&parameters, &param);
      }
      darray_free(&list, free_parsed);
      ParsedValueReturn parsedBody = parse_token(file, tokens, index, true);
      if (parsedBody.err.exists) {
        darray_free(&parameters, free_parameter);
        return parsedBody;
      }
      return (ParsedValueReturn){
          no_err,
          create_parsed_function("anonymous", parameters, parsedBody.value)};
    }
  }
  if (list.size != 1) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected 1 body"),
                               NULL};
  }
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  memcpy(parsedValue, darray_get(&list, 0), sizeof(ParsedValue));
  darray_free(&list, NULL);
  return (ParsedValueReturn){no_err, parsedValue};
}