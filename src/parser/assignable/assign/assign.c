#include "assign.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../parser.h"
#include "../call/call.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

ParsedValueReturn parse_assign(char *file, DArray *tokens,
                               ParsedValue *assign_to, size_t *index) {
  Token *token = darray_get(tokens, *index);
  switch (assign_to->type) {
  case AST_IDENTIFIER:
  case AST_ACCESS:
    break;
  case AST_CALL:;
    ParsedCall *call = assign_to->data;
    for (size_t i = 0; i < call->args.size; i++) {
      if (((ParsedValue *)darray_get(&call->args, i))->type != AST_IDENTIFIER) {
        free_parsed(assign_to);
        free(assign_to);
        return (ParsedValueReturn){
            create_err(
                token->line, token->column, token->length, file, "Syntax Error",
                "parameter names need to start with a letter "
                "or _, "
                "only use letters, digits, or _, and can't be keywords."),
            NULL};
      }
    }
    break;
  default:;
    ArErr err = create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "can't assign to %s",
                                          ValueTypeNames[assign_to->type]);
    free_parsed(assign_to);
    free(assign_to);
    return (ParsedValueReturn){err,
                               NULL};
  }
  ParsedAssign *assign = checked_malloc(sizeof(ParsedAssign));
  assign->to = assign_to;
  assign->type = token->type;
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_ASSIGN;
  parsedValue->data = assign;
  (*index)++;
  error_if_finished(file, tokens, index);
  token = darray_get(tokens, *index);
  ParsedValueReturn from = parse_token(file, tokens, index, true);
  if (from.err.exists) {
    free_parsed(parsedValue);
    free(parsedValue);
    return from;
  }
  assign->from = from.value;
  if (!assign->from) {
    free_parsed(parsedValue);
    free(parsedValue);
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected body"),
                               NULL};
  }
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