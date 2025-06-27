#include "dynamic_array/darray.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "memory.h"
#include "parser/parser.h"
#include "runtime/runtime.h"
#include "translator/translator.h"

#include <endian.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

const char FILE_IDENTIFIER[] = "ARBI";
const uint32_t version_number = 0;

int main(int argc, char *argv[]) {
  clock_t start,end;
  double time_spent;
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
  start = clock();
  lexer(state);
  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  printf("Lexer time taken: %f seconds\n", time_spent);
  fclose(state.file);

  DArray ast;

  darray_init(&ast, sizeof(ParsedValue));


  start = clock();
  parser(path, &ast, &tokens, false);
  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  printf("Parser time taken: %f seconds\n", time_spent);
  darray_free(&tokens, free_token);

  Translated translated = init_translator();


  start = clock();
  translate(&translated, &ast);
  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  printf("Translation time taken: %f seconds\n", time_spent);

  darray_free(&ast, free_parsed);

  file = fopen("out.arbin", "wb");

  uint64_t constantsSize = (uint64_t)translated.constants.size;
  uint64_t bytecodeSize = (uint64_t)translated.bytecode.size;

  uint32_t version_number_htole32ed = htole32(version_number);
  constantsSize = htole64(constantsSize);
  bytecodeSize = htole64(bytecodeSize);

  fwrite(&FILE_IDENTIFIER, sizeof(char), strlen(FILE_IDENTIFIER), file);
  fwrite(&version_number_htole32ed, sizeof(uint32_t), 1, file);
  fwrite(&translated.registerCount, sizeof(uint8_t), 1, file);
  fwrite(&constantsSize, sizeof(uint64_t), 1, file);
  fwrite(&bytecodeSize, sizeof(uint64_t), 1, file);
  fwrite(translated.constants.data, 1, translated.constants.size, file);
  fwrite(translated.bytecode.data, translated.bytecode.element_size,
         translated.bytecode.size, file);

  fclose(file);

  generate_siphash_key();

  init_types();

  start = clock();
  runtime(translated);

  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;

  printf("Execution time taken: %f seconds\n", time_spent);

  free_translator(&translated);
  return 0;
}
