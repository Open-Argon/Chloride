#include "token.h"
#include <stdlib.h>

void free_token(void *ptr) {
  Token *token = ptr;
  free(token->value);
}