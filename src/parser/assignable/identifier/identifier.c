/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "identifier.h"
#include "../../../lexer/token.h"
#include "../../parser.h"
#include <string.h>
#include "../../../memory.h"



ParsedValueReturn parse_identifier(Token *token) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedIdentifier *parsedIdentifier = checked_malloc(sizeof(ParsedIdentifier));
  parsedIdentifier->name = strcpy(checked_malloc(token->length + 1), token->value);
  parsedIdentifier->line = token->line;
  parsedIdentifier->column = token->column;
  parsedValue->type = AST_IDENTIFIER;
  parsedValue->data = parsedIdentifier;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_identifier(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedIdentifier *parsed = parsedValue->data;
  free(parsed->name);
  free(parsed);
}