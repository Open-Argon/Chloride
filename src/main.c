#include "dynamic_array/darray.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "memory.h"
#include "parser/parser.h"
#include "runtime/runtime.h"
#include "translator/translator.h"

#include "external/xxhash/xxhash.h"
#include "hash_data/hash_data.h"
#include <arpa/inet.h>
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

uint64_t htonll(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return ((uint64_t)htonl(x & 0xFFFFFFFF) << 32) | htonl(x >> 32);
#else
  return x;
#endif
}

uint64_t ntohll(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return ((uint64_t)ntohl(x & 0xFFFFFFFF) << 32) | ntohl(x >> 32);
#else
  return x;
#endif
}

int main(int argc, char *argv[]) {
  generate_siphash_key(siphash_key);
  clock_t start, end;
  double time_spent, total_time_spent = 0;
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

  XXH3_state_t *hash_state = XXH3_createState();
  XXH3_64bits_reset(hash_state);

  char buffer[8192];
  size_t bytes;
  while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    XXH3_64bits_update(hash_state, buffer, bytes);
  }
  rewind(file);
  uint64_t hash = XXH3_64bits_digest(hash_state);
  XXH3_freeState(hash_state);
  printf("Hash: %016llx\n", (unsigned long long)hash);

  LexerState state = {path, file, 0, 0, &tokens};
  start = clock();
  lexer(state);
  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  total_time_spent += time_spent;
  printf("Lexer time taken: %f seconds\n", time_spent);
  fclose(state.file);

  DArray ast;

  darray_init(&ast, sizeof(ParsedValue));

  start = clock();
  parser(path, &ast, &tokens, false);
  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  total_time_spent += time_spent;
  printf("Parser time taken: %f seconds\n", time_spent);
  darray_free(&tokens, free_token);

  Translated translated = init_translator();

  start = clock();
  translate(&translated, &ast);
  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  total_time_spent += time_spent;
  printf("Translation time taken: %f seconds\n", time_spent);

  darray_free(&ast, free_parsed);

  file = fopen("out.arbin", "wb");

  uint64_t constantsSize = (uint64_t)translated.constants.size;
  uint64_t bytecodeSize = (uint64_t)translated.bytecode.size;

  uint32_t version_number_htole32ed = htole32(version_number);
  uint64_t net_hash = htonll(hash);
  constantsSize = htole64(constantsSize);
  bytecodeSize = htole64(bytecodeSize);

  fwrite(&FILE_IDENTIFIER, sizeof(char), strlen(FILE_IDENTIFIER), file);
  fwrite(&net_hash, sizeof(net_hash), 1, file);
  fwrite(&version_number_htole32ed, sizeof(uint32_t), 1, file);
  fwrite(&translated.registerCount, sizeof(uint8_t), 1, file);
  fwrite(&constantsSize, sizeof(uint64_t), 1, file);
  fwrite(&bytecodeSize, sizeof(uint64_t), 1, file);
  fwrite(translated.constants.data, 1, translated.constants.size, file);
  fwrite(translated.bytecode.data, translated.bytecode.element_size,
         translated.bytecode.size, file);

  fclose(file);

  init_types();

  start = clock();
  runtime(translated);

  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  total_time_spent += time_spent;
  printf("Execution time taken: %f seconds\n", time_spent);
  printf("total time taken: %f seconds\n", total_time_spent);

  free_translator(&translated);
  return 0;
}
