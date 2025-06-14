#include "dynamic_array/darray.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "memory.h"
#include "parser/parser.h"
#include "translator/translator.h"

#include <endian.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

const char FILE_IDENTIFIER[] = "ARBI";
const uint64_t version_number = 0;

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  if (argc <= 1)
    return -1;
  ar_memory_init();
  char *path = argv[1];
  DArray tokens;

  darray_init(&tokens, sizeof(Token));

  FILE *file = fopen(path, "r");

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

  file = fopen("out.arbin", "wb");

  uint64_t regCount = (uint64_t)translated.registerCount;
  uint64_t constantsSize = (uint64_t)translated.constants.size;
  uint64_t bytecodeSize = (uint64_t)translated.bytecode.size;

  uint64_t version_number_htole64ed = htole64(version_number);
  regCount = htole64(regCount);
  regCount = htole64(regCount);
  constantsSize = htole64(constantsSize);
  bytecodeSize = htole64(bytecodeSize);
  
  fwrite(&FILE_IDENTIFIER, sizeof(char), strlen(FILE_IDENTIFIER), file);
  fwrite(&version_number_htole64ed, sizeof(uint64_t), 1, file);
  fwrite(&regCount, sizeof(uint64_t), 1, file);
  fwrite(&constantsSize, sizeof(uint64_t), 1, file);
  fwrite(&bytecodeSize, sizeof(uint64_t), 1, file);
  fwrite(translated.constants.data, 1, translated.constants.size, file);
  fwrite(translated.bytecode.data, translated.bytecode.element_size,
         translated.bytecode.size, file);

  fclose(file);

  free_translator(&translated);
  return 0;
}
