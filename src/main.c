/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "err.h"
#include "import.h"
#include "memory.h"
#include "runtime/internals/hashmap/hashmap.h"
#include "runtime/objects/literals/literals.h"
#include "runtime/objects/object.h"
#include "runtime/objects/string/string.h"
#include "runtime/runtime.h"
#include "shell.h"

#include "hash_data/hash_data.h"
#include <locale.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif

atomic_int thread_count = 0;

char *
get_current_directory() {
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

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  ar_memory_init();
  generate_siphash_key(siphash_key);
  init_built_in_field_hashes();
  bootstrap_types();
  bootstrap_globals();

  imported_hash_table = createHashmap_GC();
  importing_hash_table = createHashmap_GC();
  runtime_hash_table = createHashmap_GC();
  CWD = get_current_directory();
  EXC = get_executable_path();
  CWD_ARGON = CWD?new_string_object_null_terminated(CWD):ARGON_NULL;
  EXC_ARGON = EXC?new_string_object_null_terminated(EXC):ARGON_NULL;
  if (argc <= 1)
    return shell();
  char *path_non_absolute = argv[1];
  ArErr err = no_err;
  ar_import(CWD, path_non_absolute, &err, true);
  if (err.exists) {
    output_err(err);
    return 1;
  }
  free(CWD);
  ar_memory_shutdown();
  if (err.exists) {
    output_err(err);
    return 1;
  }
  // Your main thread code
  return 0;
}