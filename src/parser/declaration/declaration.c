#include "declaration.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include "../literals/literals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValue *parse_declaration(char *file, DArray *tokens,
                              size_t *index) {
  (*index)++;
  error_if_finished(file, tokens, index);
  Token *token = darray_get(tokens, *index);

  ParsedValue * parsedValue = malloc(sizeof(ParsedValue));
  ParsedDeclaration * declaration = malloc(sizeof(ParsedDeclaration));
  declaration->is_function = false;
  declaration->from = parse_null();
  parsedValue->data = declaration;
  parsedValue->type = AST_DECLARATION;

  if (token->type != TOKEN_IDENTIFIER) {
    fprintf(stderr, "%s:%u:%u error: declaration requires an identifier\n",
            file, token->line, token->column);
    exit(EXIT_FAILURE);
  }
  declaration->name = strcpy(checked_malloc(sizeof(token->value)), token->value);

  (*index)++;
  if ((*index) >= tokens->size) {
    return parsedValue;
  }
  token = darray_get(tokens, *index);
  if (token->type == TOKEN_LPAREN) {
    declaration->is_function = true;
    declaration->parameters = checked_malloc(sizeof(DArray));
    darray_init(declaration->parameters, sizeof(char*));
    (*index)++;
    error_if_finished(file, tokens, index);
    while(true) {
      token = darray_get(tokens, *index);
      if (token->type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "%s:%u:%u error: parameter names need to start with a letter or _, only use letters, digits, or _, and can't be keywords.\n",
            file, token->line, token->column);
        exit(EXIT_FAILURE);
      }
      darray_push(declaration->parameters, strcpy(checked_malloc(sizeof(token->value)), token->value));
      (*index)++;
      error_if_finished(file, tokens, index);
      token = darray_get(tokens, *index);
      if (token->type == TOKEN_RPAREN) {
        (*index)++;
        error_if_finished(file, tokens, index);
        token = darray_get(tokens, *index);
        break;
      } else if (token->type != TOKEN_COMMA) {
        fprintf(stderr, "%s:%u:%u error: expected comma\n",
            file, token->line, token->column);
        exit(EXIT_FAILURE);
      }
      (*index)++;
      error_if_finished(file, tokens, index);
    }
  }
  if (token->type != TOKEN_ASSIGN) return parsedValue;

  (*index)++;
  error_if_finished(file, tokens, index);
  
  free(declaration->from);

  declaration->from = parse_token(file, tokens, index, true);

  return parsedValue;
}