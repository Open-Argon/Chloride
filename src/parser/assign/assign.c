#include "assign.h"
#include "../../lexer/token.h"
#include "../parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

ParsedValue *parse_assign(char*file,DArray *parsed, DArray *tokens,
                          ParsedValue *assign_to, size_t *index) {
  Token *token = darray_get(tokens, *index);
  ParsedAssign *assign = malloc(sizeof(ParsedAssign));
  assign->to = assign_to;
  assign->type = token->type;
  (*index)++;
  token = darray_get(tokens, *index);
  switch (token->type) {
  case TOKEN_ASSIGN:
  case TOKEN_ASSIGN_CARET:
  case TOKEN_ASSIGN_FLOORDIV:
  case TOKEN_ASSIGN_MINUS:
  case TOKEN_ASSIGN_MODULO:
  case TOKEN_ASSIGN_PLUS:
  case TOKEN_ASSIGN_SLASH:
  case TOKEN_ASSIGN_STAR:
    fprintf(stderr, "%s:%u:%u error: invalid syntax\n", file, token->line,
            token->column);
      exit(EXIT_FAILURE);
    default:
      break;
  }
  assign->from = parse_token(file,parsed, tokens, index, true);
  ParsedValue *parsedValue = malloc(sizeof(ParsedValue));
  parsedValue->type = AST_ASSIGN;
  parsedValue->data = assign;
  return parsedValue;
}

void free_parse_assign(void*ptr) {
  ParsedValue * parsedValue = ptr;
  ParsedAssign* parsedAssign = parsedValue->data;
  free_parsed(parsedAssign->to);
  free_parsed(parsedAssign->from);
  free(parsedAssign);
}