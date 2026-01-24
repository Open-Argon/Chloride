/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "class.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include "../../err.h"
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_class(char *file, DArray *tokens, size_t *index) {
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }
  Token *first_token = darray_get(tokens, *index);
  if (first_token->type != TOKEN_IDENTIFIER) {
    return (ParsedValueReturn){create_err(first_token->line, first_token->column,
                                          first_token->length, file, "Syntax Error",
                                          "expected identifier"),
                               NULL};
  }
  char *name = first_token->value;
  ParsedValue *parent = NULL;
  (*index)++;
  err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }
  Token *token = darray_get(tokens, *index);
  if (token->type == TOKEN_LPAREN) {
    (*index)++;
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (err.exists) {
      return (ParsedValueReturn){err, NULL};
    }
    ParsedValueReturn parsedParent = parse_token(file, tokens, index, true);
    if (parsedParent.err.exists) {
      return parsedParent;
    } else if (!parsedParent.value) {
      return (ParsedValueReturn){create_err(token->line, token->column,
                                            token->length, file, "Syntax Error",
                                            "expected a value"),
                                 NULL};
    }
    parent = parsedParent.value;
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (err.exists) {
      return (ParsedValueReturn){err, NULL};
    }
    token = darray_get(tokens, *index);
    if (token->type != TOKEN_RPAREN) {
      return (ParsedValueReturn){create_err(token->line, token->column,
                                            token->length, file, "Syntax Error",
                                            "expected closing parentheses"),
                                 NULL};
    }
    (*index)++;
  }

  ParsedValueReturn parsedBody = parse_token(file, tokens, index, true);
  if (parsedBody.err.exists) {
    return parsedBody;
  } else if (!parsedBody.value) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected body"),
                               NULL};
  }

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedClass *parsedClass = checked_malloc(sizeof(ParsedClass));
  parsedClass->name = strcpy(checked_malloc(strlen(name) + 1), name);
  parsedClass->parent = parent;
  parsedClass->line = first_token->line;
  parsedClass->column = first_token->column;
  parsedClass->body = parsedBody.value;
  parsedValue->type = AST_CLASS;
  parsedValue->data = parsedClass;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_class(void *ptr) {
  ParsedValue *parsedValue = ptr;

  ParsedClass *class = parsedValue->data;
  free(class->name);
  free(class->parent);
  free_parsed(class->body);
  free(class->body);
  free(class);
}