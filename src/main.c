/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "arobject.h"
#include "dynamic_array/darray.h"
#include "hashmap/hashmap.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "memory.h"
#include "parser/parser.h"
#include "runtime/runtime.h"
#include "translator/translator.h"

#include "../external/xxhash/xxhash.h"
#include "hash_data/hash_data.h"
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
#include <direct.h>   // for _mkdir
#include <sys/stat.h> // for _stat
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
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
#include <pthread.h>

#if defined(_WIN32) || defined(_WIN64)

// Windows / MinGW usually uses little-endian, so these can be no-ops
// But define them explicitly to avoid implicit declaration warnings

static inline uint32_t le32toh(uint32_t x) { return x; }
static inline uint64_t le64toh(uint64_t x) { return x; }
static inline uint32_t htole32(uint32_t x) { return x; }
static inline uint64_t htole64(uint64_t x) { return x; }

#elif defined(__linux__)
#include <endian.h>
#include <malloc.h>
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
// Add others as needed
#else
#error "Unsupported platform"
#endif

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
#ifdef _WIN32
  struct _stat st;
  if (_stat(path, &st) != 0) {
    // Directory does not exist, create it
    if (_mkdir(path) != 0) {
      perror("_mkdir failed");
      return -1;
    }
  } else if (!(st.st_mode & _S_IFDIR)) {
    fprintf(stderr, "Path exists but is not a directory\n");
    return -1;
  }
#else
  struct stat st;
  if (stat(path, &st) != 0) {
    // Directory does not exist, create it
    if (mkdir(path, 0755) != 0) {
      perror("mkdir failed");
      return -1;
    }
  } else if (!S_ISDIR(st.st_mode)) {
    fprintf(stderr, "Path exists but is not a directory\n");
    return -1;
  }
#endif
  return 0;
}

static inline void write_and_hash(FILE *file, XXH64_state_t *state,
                                  const void *ptr, size_t size, size_t count) {
  fwrite(ptr, size, count, file);
  XXH64_update(state, ptr, size * count);
}

static inline void update_hash_from_file(FILE *file, XXH64_state_t *state,
                                         size_t size) {
  char buffer[4096];
  size_t bytes_read;
  size_t remaining = size;

  while (remaining > 0 &&
         (bytes_read =
              fread(buffer, 1,
                    remaining > sizeof(buffer) ? sizeof(buffer) : remaining,
                    file)) > 0) {
    XXH64_update(state, buffer, bytes_read);
    remaining -= bytes_read;
  }
}

const char CACHE_FOLDER[] = "__arcache__";
const char FILE_IDENTIFIER[5] = "ARBI";
const char BYTECODE_EXTENTION[] = "arbin";
const uint32_t version_number = 0;

int load_cache(Translated *translated_dest, char *joined_paths, uint64_t hash,
               char *source_path) {
  FILE *bytecode_file = fopen(joined_paths, "rb");
  if (!bytecode_file) {
    fprintf(stderr,"cache doesnt exist... compiling from source.\n");
    return 1;
  }

  // Find file size
  fseek(bytecode_file, 0, SEEK_END);
  long file_size = ftell(bytecode_file);
  if (file_size < (long)sizeof(uint64_t)) {
    goto FAILED;
  }
  fseek(bytecode_file, 0, SEEK_SET);

  // Footer is the last 8 bytes
  long data_size = file_size - sizeof(uint64_t);

  // Set up hash state
  XXH64_state_t *state = XXH64_createState();
  XXH64_reset(state, 0);

  // Hash everything except last 8 bytes
  update_hash_from_file(bytecode_file, state, data_size);

  // Read stored footer hash
  uint64_t stored_hash_le;
  if (fread(&stored_hash_le, 1, sizeof(stored_hash_le), bytecode_file) !=
      sizeof(stored_hash_le)) {
    XXH64_freeState(state);
    goto FAILED;
  }
  uint64_t stored_hash = le64toh(stored_hash_le);

  // Compare
  uint64_t calc_hash = XXH64_digest(state);
  XXH64_freeState(state);

  if (calc_hash != stored_hash) {
    fprintf(stderr,"cache hash mismatch (corrupted?)\n");
    goto FAILED;
  }

  // Now actually parse the file contents
  fseek(bytecode_file, 0, SEEK_SET); // rewind to start

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

  *translated_dest = init_translator(source_path);

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

  fprintf(stderr,"cache exists and is valid, so will be used.\n");
  fclose(bytecode_file);
  return 0;
FAILED:
  fprintf(stderr,"cache is invalid... compiling from source.\n");
  fclose(bytecode_file);
  return 1;
}

Execution execute(char *path, Stack *stack) {
  clock_t start, end;
  double time_spent, total_time_spent = 0;

  const char *basename_ptr;
  size_t basename_length;
  cwk_path_get_basename(path, &basename_ptr, &basename_length);

  if (!basename_ptr)
    return (Execution){create_err(0, 0, 0, NULL, "Path Error",
                                  "path has no basename '%s'", path),
                       (Stack){NULL, NULL}};

  char basename[FILENAME_MAX];
  memcpy(basename, basename_ptr, basename_length);

  size_t parent_directory_length;
  cwk_path_get_dirname(path, &parent_directory_length);

  char parent_directory[FILENAME_MAX];
  memcpy(parent_directory, path, parent_directory_length);
  parent_directory[parent_directory_length] = '\0';

  char cache_folder_path[FILENAME_MAX];
  cwk_path_join(parent_directory, CACHE_FOLDER, cache_folder_path,
                sizeof(cache_folder_path));

  char cache_file_path[FILENAME_MAX];
  cwk_path_join(cache_folder_path, basename, cache_file_path,
                sizeof(cache_file_path));
  cwk_path_change_extension(cache_file_path, BYTECODE_EXTENTION,
                            cache_file_path, sizeof(cache_file_path));

  FILE *file = fopen(path, "r");
  if (!file) {
    return (Execution){create_err(0, 0, 0, NULL, "File Error",
                                  "Unable to open file '%s'", path),
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

  Translated translated;

  if (load_cache(&translated, cache_file_path, hash, path) != 0) {

    DArray tokens;
    darray_init(&tokens, sizeof(Token));

    LexerState state = {path, file, 0, 0, &tokens};
    start = clock();
    ArErr err = lexer(state);
    if (err.exists) {
      darray_free(&tokens, free_token);
      return (Execution){err, (Stack){NULL, NULL}};
    }
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    total_time_spent += time_spent;
    fprintf(stderr,"Lexer time taken: %f seconds\n", time_spent);
    fclose(state.file);

    DArray ast;

    darray_init(&ast, sizeof(ParsedValue));

    start = clock();
    err = parser(path, &ast, &tokens, false);
    if (err.exists) {
      darray_free(&tokens, free_token);
      darray_free(&ast, free_parsed);
      return (Execution){err, (Stack){NULL, NULL}};
    }
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    total_time_spent += time_spent;
    fprintf(stderr,"Parser time taken: %f seconds\n", time_spent);
    darray_free(&tokens, free_token);

    start = clock();

    translated = init_translator(path);
    err = translate(&translated, &ast);
    if (err.exists) {
      darray_free(&translated.bytecode, NULL);
      free(translated.constants.data);
      hashmap_free(translated.constants.hashmap, NULL);
      darray_free(&ast, free_parsed);
      return (Execution){err, (Stack){NULL, NULL}};
    }
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    total_time_spent += time_spent;
    fprintf(stderr,"Translation time taken: %f seconds\n", time_spent);

    darray_free(&ast, free_parsed);
#if defined(__linux__)
    malloc_trim(0);
#endif

    ensure_dir_exists(cache_folder_path);

    file = fopen(cache_file_path, "wb");

    uint64_t constantsSize = translated.constants.size;
    uint64_t bytecodeSize = translated.bytecode.size;

    uint32_t version_number_htole32ed = htole32(version_number);
    uint64_t net_hash = htole64(hash);
    constantsSize = htole64(constantsSize);
    bytecodeSize = htole64(bytecodeSize);

    XXH64_state_t *hash_state = XXH64_createState();
    XXH64_reset(hash_state, 0);

    write_and_hash(file, hash_state, &FILE_IDENTIFIER, sizeof(char),
                   strlen(FILE_IDENTIFIER));
    write_and_hash(file, hash_state, &version_number_htole32ed,
                   sizeof(uint32_t), 1);
    write_and_hash(file, hash_state, &net_hash, sizeof(net_hash), 1);
    write_and_hash(file, hash_state, &translated.registerCount, sizeof(uint8_t),
                   1);
    write_and_hash(file, hash_state, &constantsSize, sizeof(uint64_t), 1);
    write_and_hash(file, hash_state, &bytecodeSize, sizeof(uint64_t), 1);
    write_and_hash(file, hash_state, translated.constants.data, 1,
                   translated.constants.size);
    write_and_hash(file, hash_state, translated.bytecode.data,
                   translated.bytecode.element_size, translated.bytecode.size);

    // Finalize the hash
    uint64_t file_hash = XXH64_digest(hash_state);
    XXH64_freeState(hash_state);

    // Convert to little-endian before writing if needed
    uint64_t file_hash_le = htole64(file_hash);
    fwrite(&file_hash_le, sizeof(file_hash_le), 1, file);

    fclose(file);
  }
  hashmap_free(translated.constants.hashmap, NULL);
  Translated gc_translated = {
      translated.registerCount, NULL, {}, {}, translated.path};
  gc_translated.bytecode.data = ar_alloc(translated.bytecode.capacity);
  memcpy(gc_translated.bytecode.data, translated.bytecode.data,
         translated.bytecode.capacity);
  gc_translated.bytecode.element_size = translated.bytecode.element_size;
  gc_translated.bytecode.size = translated.bytecode.size;
  gc_translated.bytecode.resizable = false;
  gc_translated.bytecode.capacity =
      translated.bytecode.size * translated.bytecode.element_size;
  gc_translated.constants.data = ar_alloc(translated.constants.capacity);
  memcpy(gc_translated.constants.data, translated.constants.data,
         translated.constants.capacity);
  gc_translated.constants.size = translated.constants.size;
  gc_translated.constants.capacity = translated.constants.capacity;
  darray_free(&translated.bytecode, NULL);
  free(translated.constants.data);

  start = clock();
  RuntimeState state = init_runtime_state(gc_translated, path);
  Stack *main_scope = create_scope(stack);
  ArErr err = runtime(gc_translated, state, main_scope);
  end = clock();
  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  total_time_spent += time_spent;
  fprintf(stderr,"Execution time taken: %f seconds\n", time_spent);
  fprintf(stderr,"total time taken: %f seconds\n", total_time_spent);

  return (Execution){err, *main_scope};
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  ar_memory_init();

  generate_siphash_key(siphash_key);
  bootstrap_types();
  bootstrap_globals();
  char *CWD = get_current_directory();
  if (argc <= 1)
    return -1;
  char *path_non_absolute = argv[1];
  char path[FILENAME_MAX];
  cwk_path_get_absolute(CWD, path_non_absolute, path, sizeof(path));
  free(CWD);
  Execution resp = execute(path, Global_Scope);
  if (runtime_hash_table)
    hashmap_free(runtime_hash_table, NULL);
  if (resp.err.exists) {
    output_err(resp.err);
    return 1;
  }
  // Your main thread code
  return 0;
}