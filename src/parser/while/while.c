/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "while.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../err.h"
#include "../parser.h"
#include <stddef.h>

ParsedValueReturn parse_while(char *file, DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  // Parse ( condition )
  token = darray_get(tokens, *index);
  if (token->type != TOKEN_LPAREN) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected '(' after while"),
                               NULL};
  }

  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }
  skip_newlines_and_indents(tokens, index);
  ParsedValueReturn condition = parse_token(file, tokens, index, true);
  if (condition.err.exists) {
    return condition;
  } else if (!condition.value) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected condition"),
                               NULL};
  }
  skip_newlines_and_indents(tokens, index);

  token = darray_get(tokens, *index);
  if (token->type != TOKEN_RPAREN) {
    if (condition.value) {
      free_parsed(condition.value);
      free(condition.value);
    }
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "missing closing ')' in condition"),
                               NULL};
  }

  (*index)++;
  err = error_if_finished(file, tokens, index);
  if (err.exists) {
    if (condition.value) {
      free_parsed(condition.value);
      free(condition.value);
    }
    return (ParsedValueReturn){err, NULL};
  }
  // Parse the body
  ParsedValueReturn parsed_content = parse_token(file, tokens, index, false);

  if (parsed_content.err.exists) {
    if (condition.value) {
      free_parsed(condition.value);
      free(condition.value);
    }
    return parsed_content;
  }

  if (!parsed_content.value) {
    if (condition.value) {
      free_parsed(condition.value);
      free(condition.value);
    }
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected body"),
                               NULL};
  }

  ParsedValue *Parsedvalue = checked_malloc(sizeof(ParsedValue));
  Parsedvalue->type = AST_WHILE;
  ParsedWhile *Parsed_while = checked_malloc(sizeof(ParsedWhile));
  Parsedvalue->data = Parsed_while;
  Parsed_while->condition = condition.value;
  Parsed_while->content = parsed_content.value;
  return (ParsedValueReturn){no_err, Parsedvalue};
}

void free_parsed_while(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedWhile *parsed_while = parsedValue->data;
  free_parsed(parsed_while->condition);
  free(parsed_while->condition);
  free_parsed(parsed_while->content);
  free(parsed_while->content);
  free(parsed_while);
}