/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "trycatch.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../../string/string.h"
#include <string.h>

ParsedValueReturn parse_try(char *file, DArray *tokens, size_t *index) {
  Token *token = darray_get(tokens, *index);
  (*index)++;
  skip_newlines_and_indents(tokens, index);
  ArErr err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    return (ParsedValueReturn){err, NULL};
  }
  ParsedValueReturn try_body = parse_token(file, tokens, index, false);
  if (is_error(&try_body.err)) {
    return try_body;
  } else if (!try_body.value) {
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected body"),
        NULL};
  }
  skip_newlines_and_indents(tokens, index);
  err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    free_parsed(try_body.value);
    free(try_body.value);
    return (ParsedValueReturn){err, NULL};
  }

  token = darray_get(tokens, *index);
  if (token->type != TOKEN_CATCH) {
    free_parsed(try_body.value);
    free(try_body.value);
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "missing catch keyword"),
        NULL};
  }

  (*index)++;
  err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    free_parsed(try_body.value);
    free(try_body.value);
    return (ParsedValueReturn){err, NULL};
  }

  ParsedValue *exception_type = NULL;
  char *exception_name = NULL;

  token = darray_get(tokens, *index);
  if (token->type == TOKEN_LPAREN) {
    (*index)++;
    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
      free_parsed(try_body.value);
      free(try_body.value);
      return (ParsedValueReturn){err, NULL};
    }
    ParsedValueReturn type = parse_token(file, tokens, index, false);
    if (is_error(&type.err)) {
      free_parsed(try_body.value);
      free(try_body.value);
      return type;
    } else if (!type.value) {
      free_parsed(try_body.value);
      free(try_body.value);
      return (ParsedValueReturn){
          path_specific_create_err(token->line, token->column, token->length,
                                   file, SyntaxError, "expected value"),
          NULL};
    }
    exception_type = type.value;

    skip_newlines_and_indents(tokens, index);
    err = error_if_finished(file, tokens, index);
    if (is_error(&err)) {
      free_parsed(try_body.value);
      free(try_body.value);
      free_parsed(exception_type);
      free(exception_type);
      return (ParsedValueReturn){err, NULL};
    }

    token = darray_get(tokens, *index);
    if (token->type == TOKEN_AS) {
      (*index)++;
      skip_newlines_and_indents(tokens, index);
      err = error_if_finished(file, tokens, index);
      if (is_error(&err)) {
        free_parsed(try_body.value);
        free(try_body.value);
        free_parsed(exception_type);
        free(exception_type);
        return (ParsedValueReturn){err, NULL};
      }
      token = darray_get(tokens, *index);
      if (token->type != TOKEN_IDENTIFIER) {
        free_parsed(try_body.value);
        free(try_body.value);
        free_parsed(exception_type);
        free(exception_type);
        return (ParsedValueReturn){
            path_specific_create_err(token->line, token->column, token->length,
                                     file, SyntaxError, "expected identifier"),
            NULL};
      }
      exception_name = cloneString(token->value);
    }
  }

  // Parse the body
  ParsedValueReturn catch_body = parse_token(file, tokens, index, false);

  if (is_error(&catch_body.err)) {
    if (exception_type) {
      free_parsed(exception_type);
      free(exception_type);
    }
    if (exception_name) {
      free(exception_name);
    }
    free_parsed(try_body.value);
    free(try_body.value);
    return catch_body;
  }

  if (!catch_body.value) {
    if (exception_type) {
      free_parsed(exception_type);
      free(exception_type);
    }
    if (exception_name) {
      free(exception_name);
    }
    free_parsed(try_body.value);
    free(try_body.value);
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected body"),
        NULL};
  }

  ParsedValue *Parsedvalue = checked_malloc(sizeof(ParsedValue));
  Parsedvalue->type = AST_TRY;
  ParsedTry *Parsed_try = checked_malloc(sizeof(ParsedTry));
  Parsedvalue->data = Parsed_try;
  Parsed_try->try_body = try_body.value;
  Parsed_try->catch_body = catch_body.value;
  Parsed_try->exception_type = exception_type;
  Parsed_try->exception_name = exception_name;
  return (ParsedValueReturn){no_err, Parsedvalue};
}

void free_parsed_try(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedTry *parsed_try = parsedValue->data;
  free_parsed(parsed_try->try_body);
  free(parsed_try->try_body);
  free_parsed(parsed_try->catch_body);
  free(parsed_try->catch_body);
  if (parsed_try->exception_type) {
    free_parsed(parsed_try->exception_type);
    free(parsed_try->exception_type);
  }
  if (parsed_try->exception_name)
    free(parsed_try->exception_name);
  free(parsed_try);
}