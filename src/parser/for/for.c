/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "for.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../string/string.h"
#include "../../memory.h"
#include "../parser.h"
#include <stddef.h>

ParsedValueReturn parse_for(char *file, DArray *tokens, size_t *index) {
  (*index)++;
  // Parse ( iterator )
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_LPAREN) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected '(' after for"),
                               NULL};
  }

  (*index)++;
  skip_newlines_and_indents(tokens, index);
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }
  token = darray_get(tokens, *index);
  if (token->type != TOKEN_IDENTIFIER) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected identifier"),
                               NULL};
  }
  char *key = token->value;
  (*index)++;
  skip_newlines_and_indents(tokens, index);
  err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }
  token = darray_get(tokens, *index);
  if (token->type != TOKEN_IN) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected 'in'"),
                               NULL};
  }
  (*index)++;
  skip_newlines_and_indents(tokens, index);
  err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }
  ParsedValueReturn iterator = parse_token(file, tokens, index, true);
  if (iterator.err.exists) {
    return iterator;
  } else if (!iterator.value) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected iterator"),
                               NULL};
  }
  skip_newlines_and_indents(tokens, index);

  token = darray_get(tokens, *index);
  if (token->type != TOKEN_RPAREN) {
    if (iterator.value) {
      free_parsed(iterator.value);
      free(iterator.value);
    }
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "missing closing ')' in iterator"),
                               NULL};
  }

  (*index)++;
  err = error_if_finished(file, tokens, index);
  if (err.exists) {
    if (iterator.value) {
      free_parsed(iterator.value);
      free(iterator.value);
    }
    return (ParsedValueReturn){err, NULL};
  }
  // Parse the body
  ParsedValueReturn parsed_content = parse_token(file, tokens, index, false);

  if (parsed_content.err.exists) {
    if (iterator.value) {
      free_parsed(iterator.value);
      free(iterator.value);
    }
    return parsed_content;
  }

  if (!parsed_content.value) {
    if (iterator.value) {
      free_parsed(iterator.value);
      free(iterator.value);
    }
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected body"),
                               NULL};
  }

  ParsedValue *Parsedvalue = checked_malloc(sizeof(ParsedValue));
  Parsedvalue->type = AST_FOR;
  ParsedFor *Parsed_for = checked_malloc(sizeof(ParsedFor));
  Parsedvalue->data = Parsed_for;
  Parsed_for->key = cloneString(key);
  Parsed_for->iterator = iterator.value;
  Parsed_for->content = parsed_content.value;
  return (ParsedValueReturn){no_err, Parsedvalue};
}

void free_parsed_for(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedFor *parsed_for = parsedValue->data;
  free(parsed_for->key);
  free_parsed(parsed_for->iterator);
  free(parsed_for->iterator);
  free_parsed(parsed_for->content);
  free(parsed_for->content);
  free(parsed_for);
}