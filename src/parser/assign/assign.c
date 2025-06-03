#include "assign.h"
#include "../../lexer/token.h"
#include "../parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../memory.h"

ParsedValue *parse_assign(char *file, DArray *parsed, DArray *tokens,
                          ParsedValue *assign_to, size_t *index) {
  Token *token = darray_get(tokens, *index);
  switch (assign_to->type) {
    case AST_IDENTIFIER:
    case AST_ASSIGN:
      break;
    default:
      fprintf(stderr, "%s:%u:%u error: can't assign to %s\n", file, token->line,
              token->column, ValueTypeNames[assign_to->type]);
      exit(EXIT_FAILURE);
  }
  ParsedAssign *assign = checked_malloc(sizeof(ParsedAssign));
  assign->to = assign_to;
  assign->type = token->type;
  (*index)++;
  error_if_finished(file,tokens,index);
  token = darray_get(tokens, *index);
  assign->from = parse_token(file, parsed, tokens, index, true);
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_ASSIGN;
  parsedValue->data = assign;
  return parsedValue;
}

void free_parse_assign(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedAssign *parsedAssign = parsedValue->data;
  free_parsed(parsedAssign->to);
  free_parsed(parsedAssign->from);
  free(parsedAssign);
}