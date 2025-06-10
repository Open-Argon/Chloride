#include "dynamic_array/darray.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "memory.h"
#include "parser/parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc <= 1)
    return -1;
  ar_memory_init();
  char *path = argv[1];
  DArray tokens;

  darray_init(&tokens, sizeof(Token));

  FILE * file = fopen(path, "r");

  if (!file) {
    return -1;
  }

  LexerState state = {path, file, 0, 0, &tokens};
  lexer(state);
  fclose(state.file);

  DArray ast;

  darray_init(&ast, sizeof(ParsedValue));

  parser(path, &ast, &tokens, false);
  darray_free(&tokens, free_token);

  darray_free(&ast, free_parsed);
  return 0;
}
