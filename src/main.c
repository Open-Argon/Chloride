#include "dynamic_array/darray.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "memory.h"
#include "parser/parser.h"
#include "runtime/runtime.h"
#include "translator/translator.h"

#include "../external/xxhash/xxhash.h"
#include "hash_data/hash_data.h"
#include <arpa/inet.h>
#include <endian.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include <string.h>

int ensure_dir_exists(const char *path) {
  struct stat st = {0};

  if (stat(path, &st) == -1) {
    // Directory does not exist, create it
    if (mkdir(path, 0755) != 0) {
      perror("mkdir failed");
      return -1;
    }
  }
  return 0;
}

char *normalize_path(char *path) {
#ifdef _WIN32
  for (char *p = path; *p; p++) {
    if (*p == '/') {
      *p = '\\';
    }
  }
#endif
  return path;
}

// Join two paths using '/' as separator, no platform checks here.
int path_join(char *dest, size_t dest_size, const char *path1,
              const char *path2) {
  size_t len1 = strlen(path1);
  size_t len2 = strlen(path2);

  // Check if buffer is large enough (extra 2 for '/' and '\0')
  if (len1 + len2 + 2 > dest_size)
    return -1;

  strcpy(dest, path1);

  // Add '/' if needed
  if (len1 > 0 && dest[len1 - 1] != '/') {
    dest[len1] = '/';
    dest[len1 + 1] = '\0';
    len1++;
  }

  // Skip leading '/' in path2 to avoid double separator
  if (len2 > 0 && path2[0] == '/') {
    path2++;
    len2--;
  }

  strcat(dest, path2);

  return 0;
}

const char CACHE_FOLDER[] = "__arcache__";
const char FILE_IDENTIFIER[5] = "ARBI";
const char BYTECODE_EXTENTION[] = "arbin";
const uint32_t version_number = 0;

#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

char *replace_extension(const char *path, const char *new_ext) {
  // Defensive: if new_ext doesn't start with '.', add it
  int need_dot = (new_ext[0] != '.');

  // Find last path separator to avoid changing dots in folder names
  const char *last_sep = strrchr(path, PATH_SEP);
#ifdef _WIN32
  // Windows can have '/' too as separator in practice, check it
  const char *last_alt_sep = strrchr(path, '/');
  if (last_alt_sep && (!last_sep || last_alt_sep > last_sep)) {
    last_sep = last_alt_sep;
  }
#endif

  // Find last '.' after last_sep (if any)
  const char *last_dot = strrchr(path, '.');
  if (last_dot && (!last_sep || last_dot > last_sep)) {
    // Extension found: copy path up to last_dot, then append new_ext
    size_t base_len = last_dot - path;
    size_t ext_len = strlen(new_ext) + (need_dot ? 1 : 0);
    size_t new_len = base_len + ext_len + 1;

    char *result = malloc(new_len);
    if (!result)
      return NULL;

    memcpy(result, path, base_len);

    if (need_dot)
      result[base_len] = '.';

    strcpy(result + base_len + (need_dot ? 1 : 0), new_ext);

    return result;
  } else {
    // No extension found: append '.' + new_ext (if needed)
    size_t path_len = strlen(path);
    size_t ext_len = strlen(new_ext) + (need_dot ? 1 : 0);
    size_t new_len = path_len + ext_len + 1;

    char *result = malloc(new_len);
    if (!result)
      return NULL;

    strcpy(result, path);

    if (need_dot)
      strcat(result, ".");

    strcat(result, new_ext);

    return result;
  }
}

int load_cache(Translated *translated_dest, char *joined_paths, uint64_t hash) {
  FILE *bytecode_file = fopen(joined_paths, "rb");
  if (!bytecode_file)
    return 1;
  char file_identifier_from_cache[sizeof(FILE_IDENTIFIER)];
  file_identifier_from_cache[strlen(FILE_IDENTIFIER)] = '\0';
  if (fread(&file_identifier_from_cache, 1,
            sizeof(file_identifier_from_cache) - 1,
            bytecode_file) != sizeof(file_identifier_from_cache) - 1 ||
      memcmp(file_identifier_from_cache, FILE_IDENTIFIER,
             sizeof(file_identifier_from_cache)) != 0) {
    fclose(bytecode_file);
    return 1;
  }

  uint32_t read_version;
  if (fread(&read_version, 1, sizeof(read_version), bytecode_file) !=
      sizeof(read_version)) {
    goto FAILED;
  }
  read_version = le32toh(read_version);

  if (read_version != version_number) {
    goto FAILED;
  }

  uint64_t read_hash;
  if (fread(&read_hash, 1, sizeof(read_hash), bytecode_file) !=
      sizeof(read_hash)) {
    goto FAILED;
  }
  read_hash = le64toh(read_hash);

  if (read_hash != hash) {
    goto FAILED;
  }

  uint8_t register_count;
  if (fread(&register_count, 1, sizeof(register_count), bytecode_file) !=
      sizeof(register_count)) {
    goto FAILED;
  }

  uint64_t constantsSize;
  if (fread(&constantsSize, 1, sizeof(constantsSize), bytecode_file) !=
      sizeof(constantsSize)) {
    goto FAILED;
  }
  constantsSize = le64toh(constantsSize);

  uint64_t bytecodeSize;
  if (fread(&bytecodeSize, 1, sizeof(bytecodeSize), bytecode_file) !=
      sizeof(bytecodeSize)) {
    goto FAILED;
  }
  bytecodeSize = le64toh(bytecodeSize);

  arena_resize(&translated_dest->constants, constantsSize);

  if (fread(translated_dest->constants.data, 1, constantsSize, bytecode_file) !=
      constantsSize) {
    goto FAILED;
  }

  darray_resize(&translated_dest->bytecode, bytecodeSize);

  if (fread(translated_dest->bytecode.data, 1, bytecodeSize, bytecode_file) !=
      bytecodeSize) {
    goto FAILED;
  }

  translated_dest->registerCount = register_count;

  fclose(bytecode_file);
  return 0;
FAILED:
  fclose(bytecode_file);
  return 1;
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

  char *filename_without_extention =
      replace_extension(path, BYTECODE_EXTENTION);

  size_t joined_paths_length =
      strlen(CACHE_FOLDER) + strlen(filename_without_extention) + 2;
  char *joined_paths = checked_malloc(joined_paths_length);

  path_join(joined_paths, joined_paths_length, CACHE_FOLDER,
            filename_without_extention);
  free(filename_without_extention);
  filename_without_extention = NULL;

  Translated translated = init_translator();

  if (load_cache(&translated, joined_paths, hash) != 0) {
    free_translator(&translated);
    translated = init_translator();

    DArray tokens;
    darray_init(&tokens, sizeof(Token));

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

    start = clock();
    translate(&translated, &ast);
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    total_time_spent += time_spent;
    printf("Translation time taken: %f seconds\n", time_spent);

    darray_free(&ast, free_parsed);
    ensure_dir_exists(CACHE_FOLDER);

    file = fopen(joined_paths, "wb");

    uint64_t constantsSize = (uint64_t)translated.constants.size;
    uint64_t bytecodeSize = (uint64_t)translated.bytecode.size;

    uint32_t version_number_htole32ed = htole32(version_number);
    uint64_t net_hash = htole64(hash);
    constantsSize = htole64(constantsSize);
    bytecodeSize = htole64(bytecodeSize);

    fwrite(&FILE_IDENTIFIER, sizeof(char), strlen(FILE_IDENTIFIER), file);
    fwrite(&version_number_htole32ed, sizeof(uint32_t), 1, file);
    fwrite(&net_hash, sizeof(net_hash), 1, file);
    fwrite(&translated.registerCount, sizeof(uint8_t), 1, file);
    fwrite(&constantsSize, sizeof(uint64_t), 1, file);
    fwrite(&bytecodeSize, sizeof(uint64_t), 1, file);
    fwrite(translated.constants.data, 1, translated.constants.size, file);
    fwrite(translated.bytecode.data, translated.bytecode.element_size,
           translated.bytecode.size, file);

    fclose(file);
  }

  init_types();

  start = clock();
  runtime(translated);

  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  total_time_spent += time_spent;
  printf("Execution time taken: %f seconds\n", time_spent);
  printf("total time taken: %f seconds\n", total_time_spent);

  free_translator(&translated);
  free(joined_paths);
  return 0;
}
