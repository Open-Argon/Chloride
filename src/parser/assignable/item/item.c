/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "item.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../parser.h"
#include "../../../err.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_item_access(char *file, DArray *tokens, size_t *index,
                                    ParsedValue *to_access) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedItemAccess *parsedItemAccess = checked_malloc(sizeof(ParsedItemAccess));
  parsedItemAccess->to_access = to_access;
  size_t capacity = 4;
  parsedItemAccess->items = checked_malloc(capacity * sizeof(ParsedValue *));
  parsedItemAccess->itemc = 0;
  parsedValue->type = AST_ITEM_ACCESS;
  parsedValue->data = parsedItemAccess;
  Token *token = darray_get(tokens, *index);
  parsedItemAccess->line = token->line;
  parsedItemAccess->column = token->column;
  parsedItemAccess->length = token->length;
  (*index)++;
  while (true) {
    ArErr err = error_if_finished(file, tokens, index);
    if (err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){err, NULL};
    }
    ParsedValueReturn parsedKey = parse_token(file, tokens, index, true);
    if (parsedKey.err.exists) {
      free_parsed(parsedValue);
      free(parsedValue);
      return parsedKey;
    }
    parsedItemAccess->items[parsedItemAccess->itemc++] = parsedKey.value;
    if (parsedItemAccess->itemc > capacity) {
      capacity *= 2;
      parsedItemAccess->items = checked_realloc(
          parsedItemAccess->items, capacity * sizeof(ParsedValue *));
    }
    Token *token = darray_get(tokens, *index);
    (*index)++;
    if (token->type == TOKEN_COMMA) {
      continue;
    } else if (token->type == TOKEN_RBRACKET) {
      break;
    } else {
      free_parsed(parsedValue);
      free(parsedValue);
      return (ParsedValueReturn){
          create_err(token->line, token->column, token->length, file,
                     "Syntax Error",
                     "expected either a comma or a closing bracket"),
          NULL};
    }
  }
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parse_item_access(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedItemAccess *parsedItemAccess = parsedValue->data;
  free_parsed(parsedItemAccess->to_access);
  free(parsedItemAccess->to_access);
  for (size_t i = 0; i < parsedItemAccess->itemc; i++) {
    free_parsed(parsedItemAccess->items[i]);
    free(parsedItemAccess->items[i]);
  }
  free(parsedItemAccess->items);
  free(parsedItemAccess);
}