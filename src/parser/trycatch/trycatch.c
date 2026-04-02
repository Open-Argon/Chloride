/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "trycatch.h"
#include "../../memory.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../runtime/objects/exceptions/exceptions.h"

ParsedValueReturn parse_try(char *file, DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    return (ParsedValueReturn){err, NULL};
  }
  skip_newlines_and_indents(tokens, index);
  ParsedValueReturn try_body = parse_token(file, tokens, index, true);
  if (is_error(&try_body.err)) {
    return try_body;
  } else if (!try_body.value) {
    return (ParsedValueReturn){path_specific_create_err(token->line, token->column,
                                          token->length, file, SyntaxError,
                                          "expected body"),
                               NULL};
  }
  skip_newlines_and_indents(tokens, index);

  token = darray_get(tokens, *index);
  if (token->type != TOKEN_CATCH) {
    free_parsed(try_body.value);
    free(try_body.value);
    return (ParsedValueReturn){path_specific_create_err(token->line, token->column,
                                          token->length, file, SyntaxError,
                                          "missing catch keyword"),
                               NULL};
  }

  (*index)++;
  err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    free_parsed(try_body.value);
    free(try_body.value);
    return (ParsedValueReturn){err, NULL};
  }
  // Parse the body
  ParsedValueReturn parsed_content = parse_token(file, tokens, index, false);

  if (is_error(&parsed_content.err)) {
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
    return (ParsedValueReturn){path_specific_create_err(token->line, token->column,
                                          token->length, file, SyntaxError,
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

void free_parsed_try(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedTry *parsed_try = parsedValue->data;
  free_parsed(parsed_while->try_body);
  free(parsed_while->try_body);
  free(parsed_while);
}