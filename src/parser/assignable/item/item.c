/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "item.h"
#include "../../../err.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../parser.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ArErr parse_subscript(char *file, DArray *tokens, size_t *index,
                      DArray *subscript) {
  darray_init(subscript, sizeof(ParsedValue *));
  while (true) {
    skip_newlines_and_indents(tokens, index);
    ArErr err = error_if_finished(file, tokens, index);
    if (err.exists) {
      return err;
    }
    Token *token = darray_get(tokens, *index);
    if (token->type == TOKEN_COLON) {
      (*index)++;
      void *null = NULL;
      darray_push(subscript, &null);
      continue;
    }
    ParsedValueReturn parsedKey = parse_token(file, tokens, index, true);
    if (parsedKey.err.exists) {
      return parsedKey.err;
    }
    darray_push(subscript, &parsedKey.value);
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (err.exists) {
      return err;
    }
    token = darray_get(tokens, *index);
    if (token->type == TOKEN_COLON) {
      (*index)++;
    } else {
      break;
    }
  }
  return no_err;
}

ParsedValueReturn parse_item_access(char *file, DArray *tokens, size_t *index,
                                    ParsedValue *to_access) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedItemAccess *parsedItemAccess = checked_malloc(sizeof(ParsedItemAccess));
  parsedItemAccess->to_access = to_access;
  darray_init(&parsedItemAccess->subscripts, sizeof(DArray));
  parsedValue->type = AST_ITEM_ACCESS;
  parsedValue->data = parsedItemAccess;
  Token *token = darray_get(tokens, *index);
  parsedItemAccess->line = token->line;
  parsedItemAccess->column = token->column;
  parsedItemAccess->length = token->length;
  (*index)++;
  while (true) {
    skip_newlines_and_indents(tokens, index);
    ArErr err = error_if_finished(file, tokens, index);
    if (err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }
    DArray subscript;
    err = parse_subscript(file, tokens, index, &subscript);
    if (err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }
    darray_push(&parsedItemAccess->subscripts, &subscript);
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }

    token = darray_get(tokens, *index);
    if (token->type == TOKEN_RBRACKET) {
      (*index)++;
      break;
    } else if (token->type == TOKEN_COMMA) {
      (*index)++;
    } else {
      free_parsed(parsedValue);
      free(parsedValue);

      return (ParsedValueReturn){
          create_err(token->line, token->column, token->length, file,
                     "Syntax Error", "expected comma, colon, or left bracket"),
          NULL};
    }
  }
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_subscript_item(void*ptr) {
  ParsedValue* value = *(ParsedValue**)ptr;
  if (value) {free_parsed(value);free(value);};
}

void free_subscript(void *ptr) {
  darray_free(ptr, free_subscript_item);
}

void free_parse_item_access(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedItemAccess *parsedItemAccess = parsedValue->data;

  darray_free(&parsedItemAccess->subscripts, free_subscript);

  free_parsed(parsedItemAccess->to_access);
  free(parsedItemAccess->to_access);

  free(parsedItemAccess);
}