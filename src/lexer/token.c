#include "token.h"
#include "../string/string.h"
#include <stdlib.h>

Token *create_token(TokenType type, int line, int column, char *value) {
  Token *token = malloc(sizeof(Token));
  token->type = type;
  token->line = line;
  token->column = column;
  token->value = cloneString(value);
  return token;
}

void free_token(void *ptr) {
  Token *token = ptr;
  free(token->value);
}