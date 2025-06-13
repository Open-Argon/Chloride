#include "dynamic_array/darray.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "memory.h"
#include "parser/parser.h"
#include "translator/translator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

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

  Translated translated = init_translator();

  translate(&translated, &ast);

  darray_free(&ast, free_parsed);

  file = fopen("out.car", "wb");
  
  fwrite(&translated.registerCount, sizeof(size_t), 1, file);
  fwrite(&translated.constants.size, sizeof(size_t), 1, file);
  fwrite(&translated.bytecode.size, sizeof(size_t), 1, file);
  fwrite(translated.constants.data, 1, translated.constants.size, file);
  fwrite(translated.bytecode.data, translated.bytecode.element_size, translated.bytecode.size, file);
  
  fclose(file);

  free_translator(&translated);
  return 0;
}
