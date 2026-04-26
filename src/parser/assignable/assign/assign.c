/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "assign.h"
#include "../../../err.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../../runtime/objects/exceptions/exceptions.h"
#include "../../function/function.h"
#include "../../parser.h"
#include "../../string/string.h"
#include "../access/access.h"
#include "../call/call.h"
#include "../identifier/identifier.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValueReturn parse_assign(char *file, DArray *tokens,
                               ParsedValue *assign_to, size_t *index) {
  Token *token = darray_get(tokens, *index);
  Token *assignment_token = token;
  bool is_function = false;
  char *function_name;
  bool to_free_function_name = false;
  DArray function_args;
  ParsedValue *function_assign_to;
  switch (assign_to->type) {
  case AST_IDENTIFIER:
  case AST_ACCESS:
  case AST_ITEM_ACCESS:
    break;
  case AST_CALL:;
    ParsedCall *call = assign_to->data;
    darray_init(&function_args, sizeof(char *));
    for (size_t i = 0; i < call->args.size; i++) {
      ParsedValue *arg = darray_get(&call->args, i);
      if (arg->type != AST_IDENTIFIER) {
        free_parsed(assign_to);
        free(assign_to);
        darray_free(&function_args, free_parameter);
        return (ParsedValueReturn){
            path_specific_create_err(
                token->line, token->column, token->length, file, SyntaxError,
                "parameter names need to start with a letter "
                "or _, "
                "only use letters, digits, or _, and can't be keywords."),
            NULL};
      }

      char *param = strdup(((ParsedIdentifier *)arg->data)->name);
      darray_push(&function_args, &param);
    }
    darray_free(&call->args, (void (*)(void *))free_parsed);
    is_function = true;
    function_assign_to = call->to_call;
    switch (function_assign_to->type) {
    case AST_IDENTIFIER:
      function_name = ((ParsedIdentifier *)function_assign_to->data)->name;
      break;
    case AST_ACCESS:
      if (((ParsedAccess *)function_assign_to->data)->access->type ==
          AST_STRING) {
        ParsedString *name =
            ((ParsedAccess *)function_assign_to->data)->access->data;
        function_name = checked_malloc(name->length + 1);
        function_name[name->length] = 0;
        memcpy(function_name, name->string, name->length);
        to_free_function_name = true;
        break;
      }
      // FALL THROUGH
    default:
      function_name = "anonymous";
      break;
    }
    break;
  default:;
    free_parsed(assign_to);
    free(assign_to);
    return (ParsedValueReturn){
        path_specific_create_err(
            token->line, token->column, token->length, file, SyntaxError,
            "assigning to something which can't be assigned to"),
        NULL};
  }
  (*index)++;
  ArErr err = error_if_finished(file, tokens, index);
  if (is_error(&err)) {
    free_parsed(assign_to);
    free(assign_to);
    return (ParsedValueReturn){err, NULL};
  }
  token = darray_get(tokens, *index);
  ParsedValueReturn from = parse_token(file, tokens, index, true);
  if (is_error(&from.err)) {
    free_parsed(assign_to);
    free(assign_to);
    return from;
  }
  if (!from.value) {
    free_parsed(assign_to);
    free(assign_to);
    return (ParsedValueReturn){
        path_specific_create_err(token->line, token->column, token->length,
                                 file, SyntaxError, "expected body"),
        NULL};
  }
  if (is_function) {
    from.value =
        create_parsed_function(function_name, function_args, from.value);
    if (to_free_function_name)
      free(function_name);
    free(assign_to->data);
    free(assign_to);
    assign_to = function_assign_to;
  }
  ParsedAssign *assign = checked_malloc(sizeof(ParsedAssign));
  assign->to = assign_to;
  assign->type = assignment_token->type;
  assign->line = assignment_token->line;
  assign->column = assignment_token->column;
  assign->length = assignment_token->length;
  assign->from = from.value;
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_ASSIGN;
  parsedValue->data = assign;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_parse_assign(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedAssign *parsedAssign = parsedValue->data;
  free_parsed(parsedAssign->to);
  free(parsedAssign->to);
  if (parsedAssign->from) {
    free_parsed(parsedAssign->from);
    free(parsedAssign->from);
  }
  free(parsedAssign);
}