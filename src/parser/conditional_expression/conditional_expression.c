/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "conditional_expression.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include <stddef.h>

ParsedValueReturn parse_conditional_expression(char *file, DArray *tokens,
                                               size_t *index,
                                               ParsedValue *condition) {
  (*index)++;
  Token *token = darray_get(tokens, *index);
  ParsedValueReturn parsed_true_body = parse_token(file, tokens, index, true);
  if (is_error(&parsed_true_body.err)) {
    return parsed_true_body;
  } else if (!parsed_true_body.value) {
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected value"),
        NULL};
  }
  token = darray_get(tokens, *index);
  if (token->type != TOKEN_COLON) {
    free_parsed(parsed_true_body.value);
    free(parsed_true_body.value);
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected colon"),
        NULL};
  }
  (*index)++;
  ParsedValueReturn parsed_false_body = parse_token(file, tokens, index, true);
  if (is_error(&parsed_false_body.err)) {
    free_parsed(parsed_true_body.value);
    free(parsed_true_body.value);
    return parsed_false_body;
  } else if (!parsed_false_body.value) {
    free_parsed(parsed_true_body.value);
    free(parsed_true_body.value);
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected value"),
        NULL};
  }

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedConditionalExpression *parsedConditionalExpression = checked_malloc(sizeof(ParsedConditionalExpression));
  parsedConditionalExpression->condition = condition;
  parsedConditionalExpression->true_body = parsed_true_body.value;
  parsedConditionalExpression->false_body = parsed_false_body.value;
  parsedValue->type = AST_CONDITIONAL_EXCEPTION;
  parsedValue->data = parsedConditionalExpression;
  
  return (ParsedValueReturn){no_err, parsedValue};
}


void free_expression_conditional(void *ptr) {
  ParsedValue *parsedValue = ptr;

  ParsedConditionalExpression *conditionalExpression = parsedValue->data;
  free_parsed(conditionalExpression->condition);
  free(conditionalExpression->condition);
  free_parsed(conditionalExpression->true_body);
  free(conditionalExpression->true_body);
  free_parsed(conditionalExpression->false_body);
  free(conditionalExpression->false_body);
  free(conditionalExpression);
}