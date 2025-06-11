#include "assign.h"
#include "../../../lexer/token.h"
#include "../../../memory.h"
#include "../../parser.h"
#include "../call/call.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

ParsedValue *parse_assign(char *file, DArray *tokens, ParsedValue *assign_to,
                          size_t *index) {
  Token *token = darray_get(tokens, *index);
  switch (assign_to->type) {
  case AST_IDENTIFIER:
  case AST_ACCESS:
    break;
  case AST_CALL:;
    ParsedCall *call = assign_to->data;
    for (size_t i = 0; i < call->args.size; i++) {
      if (((ParsedValue *)darray_get(&call->args, i))->type != AST_IDENTIFIER) {
        fprintf(stderr,
                "%s:%zu:%zu error: parameter names need to start with a letter "
                "or _, "
                "only use letters, digits, or _, and can't be keywords.\n",
                file, token->line, token->column);
        exit(EXIT_FAILURE);
      }
    }
    break;
  default:
    fprintf(stderr, "%s:%zu:%zu error: can't assign to %s\n", file, token->line,
            token->column, ValueTypeNames[assign_to->type]);
    exit(EXIT_FAILURE);
  }
  ParsedAssign *assign = checked_malloc(sizeof(ParsedAssign));
  assign->to = assign_to;
  assign->type = token->type;
  (*index)++;
  error_if_finished(file, tokens, index);
  token = darray_get(tokens, *index);
  assign->from = parse_token(file, tokens, index, true);
  if (!assign->from) {
    fprintf(stderr, "%s:%zu:%zu error: syntax error\n", file, token->line,
            token->column);
    exit(EXIT_FAILURE);
  }
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_ASSIGN;
  parsedValue->data = assign;
  return parsedValue;
}

void free_parse_assign(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedAssign *parsedAssign = parsedValue->data;
  free_parsed(parsedAssign->to);
  free(parsedAssign->to);
  free_parsed(parsedAssign->from);
  free(parsedAssign->from);
  free(parsedAssign);
}