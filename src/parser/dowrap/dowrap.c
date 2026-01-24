/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dowrap.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include "../../err.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *repeat_space(size_t x) {

  char *str = checked_malloc(x + 1); // +1 for the null terminator

  memset(str, ' ', x);
  str[x] = '\0';

  return str;
}

void free_string_dowrap(void *ptr) {
  // `ptr` is a pointer to a char*
  char *str = *(char **)ptr;
  free(str);
}

ParsedValueReturn parse_dowrap(char *file, DArray *tokens, size_t *index) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_DOWRAP;
  DArray *dowrap_parsed = checked_malloc(sizeof(DArray));
  darray_init(dowrap_parsed, sizeof(ParsedValue));
  parsedValue->data = dowrap_parsed;
  (*index)++;
  if ((*index) >= tokens->size)
    return (ParsedValueReturn){no_err, parsedValue};
  Token *token = darray_get(tokens, *index);
  if (token->type != TOKEN_NEW_LINE) {
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected body"),
                               NULL};
  }
  size_t indent_depth = 0;
  bool has_body_started = false;
  bool inside_body = false;
  bool pass = false;

  DArray dowrap_tokens;
  darray_init(&dowrap_tokens, sizeof(Token));
  DArray to_free;
  darray_init(&to_free, sizeof(char *));

  size_t dowrap_index = *index;
  size_t last_normal_token = dowrap_index-1;

  while (!pass && ++dowrap_index < tokens->size) {
    token = darray_get(tokens, dowrap_index);
    switch (token->type) {
    case TOKEN_NEW_LINE:
      darray_push(&dowrap_tokens, token);
      inside_body = false;
      break;
    case TOKEN_INDENT:
      if (!inside_body && !has_body_started) {
        indent_depth = token->length;
        inside_body = true;
      } else if (indent_depth < token->length) {
        size_t indent_amount = token->length - indent_depth;
        Token indent_token;
        indent_token.line = token->line;
        indent_token.column = token->column;
        indent_token.length = indent_amount;
        indent_token.type = TOKEN_INDENT;
        indent_token.value = repeat_space(indent_amount);
        darray_push(&dowrap_tokens, &indent_token);
        darray_push(&to_free, &indent_token.value);
        inside_body = true;
      } else if (indent_depth == token->length) {
        inside_body = true;
      } else if (indent_depth > token->length) {
        inside_body = false;
      }
      break;
    default:
      if (!inside_body) {
        pass = true;
        break;
      }
      last_normal_token = dowrap_index;
      has_body_started = true;
      darray_push(&dowrap_tokens, token);
    }
  }
  *index = last_normal_token + 1;
  ArErr err = parser(file, dowrap_parsed, &dowrap_tokens, false);

  darray_free(&dowrap_tokens, NULL);

  darray_free(&to_free, free_string_dowrap);

  if (err.exists) {
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){err, NULL};
  }

  return (ParsedValueReturn){no_err, parsedValue};
}

void free_dowrap(void *ptr) {
  ParsedValue *parsedValue = ptr;
  DArray *parsed = parsedValue->data;
  darray_free(parsed, free_parsed);
  free(parsed);
}