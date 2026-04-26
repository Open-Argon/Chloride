/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../external/cwalk/include/cwalk.h"
#include "arobject.h"
#include "err.h"
#include "import.h"
#include "memory.h"
#include "runtime/internals/hashmap/hashmap.h"
#include "runtime/objects/literals/literals.h"
#include "runtime/objects/object.h"
#include "runtime/objects/string/string.h"
#include "runtime/runtime.h"
#include "shell.h"

#include <locale.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif

atomic_int thread_count = 0;

int get_current_directory(char *buffer, size_t size) {

#ifdef _WIN32
  if (GetCurrentDirectoryA(size, buffer) == 0) {
    return -1;
  }
#else
  if (getcwd(buffer, size) == NULL) {
    return -1;
  }
#endif

  return 0;
}

volatile sig_atomic_t KeyboardInterrupted = 0;

void sigint_handler(int signum) { KeyboardInterrupted = signum; }

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  ar_memory_init();
  // generate_siphash_key(siphash_key);
  init_built_in_field_hashes();
  bootstrap_types();
  bootstrap_globals();

  imported_hash_table = createHashmap_GC();
  importing_hash_table = createHashmap_GC();
  // runtime_hash_table = createHashmap_GC();
  get_current_directory(CWD, sizeof(CWD));
  get_executable_path(EXC, sizeof(EXC));
  size_t dir_length;
  cwk_path_get_dirname(EXC, &dir_length);
  memcpy(EXC_DIR, EXC, dir_length);
  EXC[dir_length] = '\0';
  CWD_ARGON = new_string_object_null_terminated(CWD);
  EXC_ARGON = new_string_object_null_terminated(EXC);
  if (argc <= 1)
    return shell();
  struct sigaction sa = {0};
  sa.sa_handler = sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0; // intentionally no SA_RESTART — let syscalls return EINTR
  sigaction(SIGINT, &sa, NULL);
  char *path_non_absolute = argv[1];
  ArErr err = {.ptr = ARGON_NULL};

  ar_import(CWD, path_non_absolute, &err, true);
  if (is_error(&err)) {
    output_err(&err);
    return 1;
  }
  ar_memory_shutdown();
  // Your main thread code
  return 0;
}