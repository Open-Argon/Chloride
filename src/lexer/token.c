#include "token.h"
#include "../string/string.h"
#include <stdlib.h>
#include "../memory.h"

Token *create_token(TokenType type, int line, int column, char *value) {
  Token *token = checked_malloc(sizeof(Token));
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