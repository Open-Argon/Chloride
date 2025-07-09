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
#include "../external/cwalk/include/cwalk.h"
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif
#include "err.h"

char *get_current_directory() {
  char *buffer = NULL;

#ifdef _WIN32
  DWORD size = GetCurrentDirectoryA(0, NULL);
  buffer = malloc(size);
  if (buffer == NULL)
    return NULL;
  if (GetCurrentDirectoryA(size, buffer) == 0) {
    free(buffer);
    return NULL;
  }
#else
  long size = pathconf(".", _PC_PATH_MAX);
  if (size == -1)
    size = 4096; // fallback
  buffer = malloc(size);
  if (buffer == NULL)
    return NULL;
  if (getcwd(buffer, size) == NULL) {
    free(buffer);
    return NULL;
  }
#endif

  return buffer;
}

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

const char CACHE_FOLDER[] = "__arcache__";
const char FILE_IDENTIFIER[5] = "ARBI";
const char BYTECODE_EXTENTION[] = "arbin";
const uint32_t version_number = 0;

int load_cache(Translated *translated_dest, char *joined_paths, uint64_t hash) {
  FILE *bytecode_file = fopen(joined_paths, "rb");
  if (!bytecode_file)
    return 1;
  char file_identifier_from_cache[sizeof(FILE_IDENTIFIER)] = {0};
  if (fread(&file_identifier_from_cache, 1,
            sizeof(file_identifier_from_cache) - 1,
            bytecode_file) != sizeof(file_identifier_from_cache) - 1 ||
      memcmp(file_identifier_from_cache, FILE_IDENTIFIER,
             sizeof(file_identifier_from_cache)) != 0) {
    goto FAILED;
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

Execution execute(char *absolute_path) {
  clock_t start, end;
  double time_spent, total_time_spent = 0;

  const char *basename_ptr;
  size_t basename_length;
  cwk_path_get_basename(absolute_path, &basename_ptr, &basename_length);

  if (!basename_ptr)
    return (Execution){create_err(0, 0, 0, NULL, "Path Error",
                                  "path has no basename '%s'", absolute_path),
                       (Stack){NULL, NULL}};

  char basename[FILENAME_MAX];
  memcpy(basename, basename_ptr, basename_length);

  size_t parent_directory_length;
  cwk_path_get_dirname(absolute_path, &parent_directory_length);

  char parent_directory[FILENAME_MAX];
  memcpy(parent_directory, absolute_path, parent_directory_length);
  parent_directory[parent_directory_length] = '\0';

  char cache_folder_path[FILENAME_MAX];
  cwk_path_join(parent_directory, CACHE_FOLDER, cache_folder_path,
                sizeof(cache_folder_path));

  char cache_file_path[FILENAME_MAX];
  cwk_path_join(cache_folder_path, basename, cache_file_path,
                sizeof(cache_file_path));
  cwk_path_change_extension(cache_file_path, BYTECODE_EXTENTION,
                            cache_file_path, sizeof(cache_file_path));

  FILE *file = fopen(absolute_path, "r");
  if (!file) {
    return (Execution){create_err(0, 0, 0, NULL, "File Error",
                                  "Unable to open file '%s'", absolute_path),
                       (Stack){NULL, NULL}};
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

  Translated translated = init_translator();

  if (load_cache(&translated, cache_file_path, hash) != 0) {
    free_translator(&translated);
    translated = init_translator();

    DArray tokens;
    darray_init(&tokens, sizeof(Token));

    LexerState state = {absolute_path, file, 0, 0, &tokens};
    start = clock();
    ArErr err = lexer(state);
    if (err.exists) {
      free_translator(&translated);
      darray_free(&tokens, free_token);
      return (Execution){err, (Stack){NULL, NULL}};
    }
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    total_time_spent += time_spent;
    printf("Lexer time taken: %f seconds\n", time_spent);
    fclose(state.file);

    DArray ast;

    darray_init(&ast, sizeof(ParsedValue));

    start = clock();
    err = parser(absolute_path, &ast, &tokens, false);
    if (err.exists) {
      free_translator(&translated);
      darray_free(&tokens, free_token);
      darray_free(&ast, free_parsed);
      return (Execution){err, (Stack){NULL, NULL}};
    }
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
    ensure_dir_exists(cache_folder_path);

    file = fopen(cache_file_path, "wb");

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

  start = clock();
  RuntimeState state = init_runtime_state(translated);
  Stack main_scope = create_scope(NULL);
  ArErr err = runtime(translated, state, main_scope);

  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  total_time_spent += time_spent;
  printf("Execution time taken: %f seconds\n", time_spent);
  printf("total time taken: %f seconds\n", total_time_spent);

  free_translator(&translated);
  return (Execution){err, main_scope};
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  ar_memory_init();
  generate_siphash_key(siphash_key);
  init_types();
  char *CWD = get_current_directory();
  if (argc <= 1)
    return -1;
  char *path_non_absolute = argv[1];
  char path[FILENAME_MAX];
  cwk_path_get_absolute(CWD, path_non_absolute, path, sizeof(path));
  free(CWD);
  Execution resp = execute(path);
  if (resp.err.exists) {
    output_err(resp.err);
    return -1;
  }
  return 0;
}