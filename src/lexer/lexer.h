#ifndef LEXER_H
#define LEXER_H

#include "../dynamic_array/darray.h"
#include "token.h"
#include <stdio.h>

typedef struct {
  const char *path;
  FILE *file;
  size_t current_line;
  size_t current_column;
  DArray *tokens;
  // add more fields as needed
} LexerState;

void lexer(LexerState state);

#endif // LEXER_H