/*
 * SPDX-FileCopyrightText: 2026 2025 William Bell
 * SPDX-FileCopyrightText: 2026, 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../external/cwalk/include/cwalk.h"
#include "../external/xxhash/xxhash.h"
#include "arobject.h"
#include "err.h"
#include "hash_data/hash_data.h"
#include "hashmap/hashmap.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "runtime/internals/hashmap/hashmap.h"
#include "runtime/objects/dictionary/dictionary.h"
#include "runtime/objects/literals/literals.h"
#include "runtime/objects/string/string.h"
#include "runtime/runtime.h"
#include "memory.h"
#include "translator/translator.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>   // for _mkdir
#include <sys/stat.h> // for _stat
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int is_regular_file(const char *path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISREG(path_stat.st_mode);
}
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
// Add others as needed
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <stdlib.h>
#include <sys/endian.h>
#endif

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

char *CWD;
char *EXC;
ArgonObject *CWD_ARGON;
ArgonObject *EXC_ARGON;

const char CACHE_FOLDER[] = "__arcache__";
const char FILE_IDENTIFIER[5] = "ARBI";
const char BYTECODE_EXTENTION[] = "arbin";
const uint32_t version_number = 1;
const char version_string[] = "0.0.1";

bool file_exists(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    return S_ISREG(st.st_mode); // true only if it's a regular file
  }
  return false; // doesn't exist, or stat failed
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

int load_cache(Translated *translated_dest, char *joined_paths, uint64_t hash,
               char *source_path) {
  FILE *bytecode_file = fopen(joined_paths, "rb");
  if (!bytecode_file) {
    fprintf(stderr, "cache doesnt exist... compiling from source.\n");
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
    fprintf(stderr, "cache hash mismatch (corrupted?)\n");
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

  translated_dest->registerCount = register_count;

  arena_resize(&translated_dest->constants, constantsSize);

  if (fread(translated_dest->constants.data, 1, constantsSize, bytecode_file) !=
      constantsSize) {
    goto FAILED;
  }

  translated_dest->constants.size = constantsSize;

  darray_resize(&translated_dest->bytecode, bytecodeSize);

  if (fread(translated_dest->bytecode.data, 1, bytecodeSize, bytecode_file) !=
      bytecodeSize) {
    goto FAILED;
  }

  translated_dest->bytecode.size = bytecodeSize;

  fprintf(stderr, "cache exists and is valid, so will be used.\n");
  fclose(bytecode_file);
  return 0;
FAILED:
  fprintf(stderr, "cache is invalid... compiling from source.\n");
  fclose(bytecode_file);
  return 1;
}

Translated load_argon_file(char *path, ArErr *err) {
  clock_t start, end;
  clock_t beginning = clock();
  double time_spent, total_time_spent = 0;

  const char *basename_ptr;
  size_t basename_length;
  cwk_path_get_basename(path, &basename_ptr, &basename_length);

  if (!basename_ptr) {
    *err = create_err(0, 0, 0, NULL, "Path Error", "path has no basename '%s'",
                      path);
    return (Translated){};
  }

  char basename[PATH_MAX];
  memcpy(basename, basename_ptr, basename_length);

  size_t parent_directory_length;
  cwk_path_get_dirname(path, &parent_directory_length);

  char parent_directory[PATH_MAX];
  memcpy(parent_directory, path, parent_directory_length);
  parent_directory[parent_directory_length] = '\0';

  char cache_folder_path[PATH_MAX];
  cwk_path_join(parent_directory, CACHE_FOLDER, cache_folder_path,
                sizeof(cache_folder_path));

  char cache_file_path[PATH_MAX];
  cwk_path_join(cache_folder_path, basename, cache_file_path,
                sizeof(cache_file_path));
  cwk_path_change_extension(cache_file_path, BYTECODE_EXTENTION,
                            cache_file_path, sizeof(cache_file_path));
  FILE *file = fopen(path, "r");
  if (!file) {
    *err = create_err(0, 0, 0, NULL, "File Error", "Unable to open file '%s'",
                      path);
    return (Translated){};
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
    *err = lexer(state);
    if (err->exists) {
      darray_free(&tokens, free_token);
      return (Translated){};
    }
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    fprintf(stderr, "Lexer time taken: %f seconds\n", time_spent);
    fclose(state.file);

    DArray ast;

    darray_init(&ast, sizeof(ParsedValue));

    start = clock();
    *err = parser(path, &ast, &tokens, false);
    darray_free(&tokens, free_token);
    if (err->exists) {
      darray_free(&ast, (void (*)(void *))free_parsed);
      return (Translated){};
    }
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    fprintf(stderr, "Parser time taken: %f seconds\n", time_spent);

    start = clock();

    translated = init_translator(path);
    *err = translate(&translated, &ast);
    darray_free(&ast, (void (*)(void *))free_parsed);
    if (err->exists) {
      darray_free(&translated.bytecode, NULL);
      free(translated.constants.data);
      hashmap_free(translated.constants.hashmap, NULL);
      return (Translated){};
    }
    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    fprintf(stderr, "Translation time taken: %f seconds\n", time_spent);
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
      translated.registerCount, translated.registerAssignment, NULL, {}, {},
      translated.path};
  gc_translated.bytecode.data = ar_alloc_atomic(translated.bytecode.capacity +
                                                translated.constants.capacity);
  memcpy(gc_translated.bytecode.data, translated.bytecode.data,
         translated.bytecode.capacity);
  gc_translated.bytecode.element_size = translated.bytecode.element_size;
  gc_translated.bytecode.size = translated.bytecode.size;
  gc_translated.bytecode.resizable = false;
  gc_translated.bytecode.capacity =
      translated.bytecode.size * translated.bytecode.element_size;
  gc_translated.constants.data =
      gc_translated.bytecode.data + translated.bytecode.capacity;
  memcpy(gc_translated.constants.data, translated.constants.data,
         translated.constants.capacity);
  gc_translated.constants.size = translated.constants.size;
  gc_translated.constants.capacity = translated.constants.capacity;
  free(translated.bytecode.data);
  free(translated.constants.data);
  total_time_spent = (double)(clock() - beginning) / CLOCKS_PER_SEC;
  fprintf(stderr, "total time taken loading file (%s): %f seconds\n", path,
          total_time_spent);
  return gc_translated;
}

const char *PRE_PATHS_TO_TEST[] = {"", "", "argon_modules", "argon_modules"};
const char *POST_PATHS_TO_TEST[sizeof(PRE_PATHS_TO_TEST) / sizeof(char *)] = {
    "", "init.ar", "", "init.ar"};
const char *EXTENTIONS_TO_TEST[sizeof(PRE_PATHS_TO_TEST) / sizeof(char *)] = {
    "", "init.ar", "", "init.ar"};

struct hashmap *importing_hash_table;
struct hashmap_GC *imported_hash_table;

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <limits.h>
#include <mach-o/dyld.h>
#else // Linux / Unix
#include <limits.h>
#include <unistd.h>
#endif

char *get_executable_path() {
  static char path[PATH_MAX];

#if defined(_WIN32)
  if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) {
    return NULL;
  }
#elif defined(__APPLE__)
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) != 0) {
    return NULL; // buffer too small
  }
#else // Linux / Unix
  ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (len == -1)
    return NULL;
  path[len] = '\0';
#endif

  return path;
}

Stack *ar_import(char *current_directory, char *path_relative, ArErr *err,
                 bool is_main) {
  char path[PATH_MAX];
  bool found = false;
  for (size_t i = 0; i < sizeof(PRE_PATHS_TO_TEST) / sizeof(char *); i++) {
    cwk_path_get_absolute(current_directory, PRE_PATHS_TO_TEST[i], path,
                          sizeof(path));
    cwk_path_get_absolute(path, path_relative, path, sizeof(path));
    cwk_path_get_absolute(path, POST_PATHS_TO_TEST[i], path, sizeof(path));
    if (file_exists(path)) {
      found = true;
      break;
    }
  }
  if (!found) {
    *err = create_err(0, 0, 0, NULL, "File Error", "Unable to find file '%s'",
                      path_relative);
    return NULL;
  }
  uint64_t hash = siphash64_bytes(path, strlen(path), siphash_key);

  if (hashmap_lookup(importing_hash_table, hash)) {
    *err = create_err(0, 0, 0, NULL, "Import Error",
                      "Circular import detected: %s", path, path_relative);
    return NULL;
  }

  hashmap_insert(importing_hash_table, hash, path, (void *)true, 0);

  Translated translated = load_argon_file(path, err);
  if (err->exists) {
    hashmap_insert(importing_hash_table, hash, path, (void *)NULL, 0);
    return NULL;
  }
  clock_t start = clock(), end;
  RuntimeState state = init_runtime_state(translated, path);
  Stack *program_scope = create_scope(Global_Scope, true);
  hashmap_GC *program = createHashmap_GC();
  hashmap_GC *file = createHashmap_GC();
  add_to_hashmap(file, "path", new_string_object_null_terminated(path));
  const char *basename;
  size_t basename_length;
  cwk_path_get_basename(path, &basename, &basename_length);
  size_t dirname_length;
  cwk_path_get_dirname(path, &dirname_length);
  add_to_hashmap(file, "name",
                 new_string_object((char *)basename, basename_length, 0, 0));
  add_to_hashmap(file, "directory",
                 new_string_object(path, dirname_length, 0, 0));
  add_to_hashmap(program, "file", create_dictionary(file));
  add_to_hashmap(program, "main", is_main ? ARGON_TRUE : ARGON_FALSE);
  add_to_hashmap(program, "origin",
                 new_string_object_null_terminated(current_directory));
  add_to_hashmap(program, "cwd", CWD_ARGON);
  add_to_hashmap(program, "exc", EXC_ARGON);

  add_to_scope(program_scope, "program", create_dictionary(program));
  Stack *main_scope = create_scope(program_scope, true);
  runtime(translated, state, main_scope, err);
  if (err->exists) {
    hashmap_insert(importing_hash_table, hash, path, (void *)NULL, 0);
    return NULL;
  }
  end = clock();
  double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  fprintf(stderr, "Execution time taken: %f seconds\n", time_spent);
  hashmap_insert_GC(imported_hash_table, hash, path, main_scope, 0);
  hashmap_insert(importing_hash_table, hash, path, (void *)NULL, 0);
  return main_scope;
}