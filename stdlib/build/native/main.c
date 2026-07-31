// SPDX-FileCopyrightText: 2026 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static int compiler_exists(const char *name, char *path, size_t path_size) {
#ifdef _WIN32

  char buffer[MAX_PATH];

  DWORD result = SearchPathA(NULL, name, ".exe", sizeof(buffer), buffer, NULL);

  if (result == 0 || result >= sizeof(buffer))
    return 0;

  strncpy(path, buffer, path_size);
  path[path_size - 1] = '\0';

  return 1;

#else

  char *env_path = getenv("PATH");

  if (!env_path)
    return 0;

  char *paths = strdup(env_path);

  if (!paths)
    return 0;

  char *dir = strtok(paths, ":");

  while (dir) {
    snprintf(path, path_size, "%s/%s", dir, name);

    if (access(path, X_OK) == 0) {
      free(paths);
      return 1;
    }

    dir = strtok(NULL, ":");
  }

  free(paths);

  return 0;

#endif
}

typedef struct {
  const char *name;
  const char *executable;
  const char *vendor;
  const char *family;

  const char *languages[8];
  size_t language_count;
  int priority;
} BuildToolCandidate;
static BuildToolCandidate candidates[] = {

    // C / C++

    {"clang",
     "clang",
     "LLVM",
     "gcc",
     {"c", "cpp", "objective-c", "objective-cpp"},
     4,
     100},

    {"clang++", "clang++", "LLVM", "gcc", {"cpp"}, 1, 100},

    {"clang-cl", "clang-cl", "LLVM", "msvc", {"c", "cpp"}, 2, 90},

    {"gcc", "gcc", "GNU", "gcc", {"c", "cpp", "fortran"}, 3, 95},

    {"g++", "g++", "GNU", "gcc", {"cpp"}, 1, 85},

    {"tcc", "tcc", "TinyCC", "gcc", {"c"}, 1, 30},

    {"icc", "icc", "Intel", "gcc", {"c", "cpp"}, 2, 40},

    {"icx", "icx", "Intel", "gcc", {"c", "cpp"}, 2, 35},

    // Microsoft

    {"msvc", "cl", "Microsoft", "msvc", {"c", "cpp"}, 2, 75},

    // Zig

    {"zig", "zig", "Zig", "zigcc", {"zig", "c", "cpp"}, 3, 50},

    // Rust

    {"rustc", "rustc", "Rust Foundation", "rust", {"rust"}, 1, 50},

    // Go

    {"go", "go", "Google", "go", {"go"}, 1, 50},

    // Swift

    {"swiftc", "swiftc", "Apple", "swift", {"swift"}, 1, 50},

    // Kotlin

    {"kotlinc", "kotlinc", "JetBrains", "kotlin", {"kotlin"}, 1, 50},

    // Java

    {"javac", "javac", "Oracle", "java", {"java"}, 1, 50},

    // C#

    {"dotnet",
     "dotnet",
     "Microsoft",
     "dotnet",
     {"csharp", "fsharp", "vb"},
     3,
     50},

    // D

    {"dmd", "dmd", "D Language Foundation", "d", {"d"}, 1, 75},

    {"ldc", "ldc2", "LLVM D Compiler", "d", {"d"}, 1, 50},

    // Nim

    {"nim", "nim", "Nim", "nim", {"nim"}, 1, 50},

    // Haskell

    {"ghc", "ghc", "Haskell", "ghc", {"haskell"}, 1, 50},

    // OCaml

    {"ocamlopt", "ocamlopt", "OCaml", "ocaml", {"ocaml"}, 1, 50},

    // Fortran

    {"gfortran", "gfortran", "GNU", "gcc", {"fortran"}, 1, 50},

    // Pascal

    {"fpc", "fpc", "FreePascal", "pascal", {"pascal"}, 1, 50},

    // Ada

    {"gnat", "gnat", "GNU", "gnat", {"ada"}, 1, 50},

    // Assembly

    {"nasm", "nasm", "Netwide", "nasm", {"asm"}, 1, 75},

    {"yasm", "yasm", "Yasm", "nasm", {"asm"}, 1, 50},

    // CUDA

    {"nvcc", "nvcc", "NVIDIA", "cuda", {"cuda", "cpp"}, 2, 50},

    // WebAssembly

    {"emcc", "emcc", "Emscripten", "gcc", {"c", "cpp", "wasm"}, 3, 50},

    // JavaScript / TypeScript

    {"node", "node", "Node.js", "node", {"javascript"}, 1, 50},

    {"tsc", "tsc", "Microsoft", "typescript", {"typescript"}, 1, 50},

    // V

    {"v", "v", "V Language", "v", {"v"}, 1, 50},

    // Crystal

    {"crystal", "crystal", "Crystal", "crystal", {"crystal"}, 1, 50},

    // Julia

    {"julia", "julia", "Julia", "julia", {"julia"}, 1, 50},

    // LaTeX

    {"latex", "latex", "TeX", "latex", {"latex"}, 1, 50}

};

static ArgonObject *create_compiler(ArgonNativeAPI *api, const char *name,
                                    const char *vendor, const char *path,
                                    const char *family, const char **languages,
                                    size_t language_count, int priority) {
  ArgonHashmap *compiler = api->create_hashmap();

  api->add_to_hashmap_string_key(compiler, "name",
                                 ARGON_STRING_FROM_C_STRING((char *)name));

  api->add_to_hashmap_string_key(compiler, "vendor",
                                 ARGON_STRING_FROM_C_STRING((char *)vendor));

  api->add_to_hashmap_string_key(compiler, "path",
                                 ARGON_STRING_FROM_C_STRING((char *)path));

  api->add_to_hashmap_string_key(compiler, "family",
                                 ARGON_STRING_FROM_C_STRING((char *)family));

  ArgonObject *language_objects[8];

  for (size_t i = 0; i < language_count; i++) {
    language_objects[i] = ARGON_STRING_FROM_C_STRING((char *)languages[i]);
  }

  api->add_to_hashmap_string_key(
      compiler, "languages",
      api->create_argon_array(language_objects, language_count));

  api->add_to_hashmap_string_key(compiler, "priority",
                                 api->i64_to_argon(priority));

  return api->hashmap_to_dictionary(compiler);
}

ARGON_FUNCTION(build_detect_compilers, {
  (void)state;
  (void)argv;

  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  ArgonObject *items[16];
  size_t count = 0;

  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    char path[1024];

    if (!compiler_exists(candidates[i].executable, path, sizeof(path))) {
      continue;
    }

    items[count++] =
        create_compiler(api, candidates[i].name, candidates[i].vendor, path,
                        candidates[i].family, candidates[i].languages,
                        candidates[i].language_count, candidates[i].priority);
  }

  return api->create_argon_array(items, count);
})

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  (void)vm;
  (void)err;

  REGISTER_ARGON_FUNCTION(build_detect_compilers)
}