#include "declaration.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValue *parse_declaration(char *file, DArray *parsed, DArray *tokens,
                              size_t *index) {
  (*index)++;
  error_if_finished(file, tokens, index);
  Token *token = darray_get(tokens, *index);

  ParsedValue * parsedValue = malloc(sizeof(ParsedValue));
  ParsedDeclaration * declaration = malloc(sizeof(ParsedDeclaration));
  parsedValue->data = declaration;
  parsedValue->type = AST_DECLARATION;

  if (token->type != TOKEN_IDENTIFIER) {
    fprintf(stderr, "%s:%u:%u error: declaration requires an identifier\n",
            file, token->line, token->column);
    exit(EXIT_FAILURE);
  }
  declaration->name = strcpy(checked_malloc(sizeof(token->value)), token->value);
  (*index)++;
  if ((*index) >= tokens->size) return parsedValue;
  token = darray_get(tokens, *index);
  return parsedValue;
}