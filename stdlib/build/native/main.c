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
#include <sys/utsname.h>
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

/*
 * Returns the host architecture in the same names used by the
 * cross compiler metadata:
 *
 *   x86_64
 *   x86
 *   arm64
 *   arm
 *
 * Returns NULL when the architecture is unknown.
 */
static const char *host_arch(void) {
#ifdef _WIN32

#if defined(_M_X64) || defined(__x86_64__)
  return "x86_64";
#elif defined(_M_IX86) || defined(__i386__)
  return "x86";
#elif defined(_M_ARM64) || defined(__aarch64__)
  return "arm64";
#elif defined(_M_ARM) || defined(__arm__)
  return "arm";
#else
  return NULL;
#endif

#else

  struct utsname info;

  if (uname(&info) != 0)
    return NULL;

  if (strcmp(info.machine, "x86_64") == 0)
    return "x86_64";

  if (strcmp(info.machine, "amd64") == 0)
    return "x86_64";

  if (strcmp(info.machine, "i386") == 0 || strcmp(info.machine, "i486") == 0 ||
      strcmp(info.machine, "i586") == 0 || strcmp(info.machine, "i686") == 0)
    return "x86";

  if (strcmp(info.machine, "aarch64") == 0 ||
      strcmp(info.machine, "arm64") == 0)
    return "arm64";

  if (strcmp(info.machine, "armv7l") == 0 ||
      strcmp(info.machine, "armv8l") == 0 || strcmp(info.machine, "arm") == 0)
    return "arm";

  return NULL;

#endif
}

static const char *host_os(void) {
#ifdef _WIN32
  return "windows";
#else
  struct utsname info;
  if (uname(&info) != 0)
    return NULL;
  if (strcmp(info.sysname, "Linux") == 0)
    return "linux";
  if (strcmp(info.sysname, "Darwin") == 0)
    return "darwin";
  if (strcmp(info.sysname, "FreeBSD") == 0)
    return "freebsd";
  return NULL;
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

  /*
   * Which OS this specific executable produces binaries for, or NULL
   * if it targets whatever the host is (the common case).
   */
  const char *cross_target;

  /*
   * Which architecture this executable produces binaries for, or NULL
   * if it targets whatever the host's architecture is.
   */
  const char *cross_arch;
} BuildToolCandidate;

static BuildToolCandidate candidates[] = {

    // C / C++

    {"clang",
     "clang",
     "LLVM",
     "gcc",
     {"c", "cpp", "objective-c", "objective-cpp"},
     4,
     90,
     NULL,
     NULL},

    {"clang++", "clang++", "LLVM", "gcc", {"cpp"}, 1, 90, NULL, NULL},

    {"clang-cl", "clang-cl", "LLVM", "msvc", {"c", "cpp"}, 2, 5, NULL, NULL},

    {"gcc", "gcc", "GNU", "gcc", {"c", "cpp", "fortran"}, 3, 95, NULL, NULL},

    {"g++", "g++", "GNU", "gcc", {"cpp"}, 1, 85, NULL, NULL},

    /*
     * Linux x86_64 -> Linux ARM64
     *
     * Executable:
     *   aarch64-linux-gnu-gcc
     *
     * Target:
     *   Linux ARM64
     */
    {"aarch64-linux-gnu-gcc",
     "aarch64-linux-gnu-gcc",
     "GNU",
     "gcc",
     {"c", "cpp"},
     2,
     95,
     "linux",
     "arm64"},

    {"aarch64-linux-gnu-g++",
     "aarch64-linux-gnu-g++",
     "GNU",
     "gcc",
     {"cpp"},
     1,
     85,
     "linux",
     "arm64"},

    /*
     * Linux ARM64 -> Linux x86_64
     *
     * Executable:
     *   x86_64-linux-gnu-gcc
     *
     * Target:
     *   Linux x86_64
     */
    {"x86_64-linux-gnu-gcc",
     "x86_64-linux-gnu-gcc",
     "GNU",
     "gcc",
     {"c", "cpp"},
     2,
     95,
     "linux",
     "x86_64"},

    {"x86_64-linux-gnu-g++",
     "x86_64-linux-gnu-g++",
     "GNU",
     "gcc",
     {"cpp"},
     1,
     85,
     "linux",
     "x86_64"},

    /*
     * Linux -> Windows via mingw-w64
     */
    {"x86_64-w64-mingw32-gcc",
     "x86_64-w64-mingw32-gcc",
     "GNU (mingw-w64)",
     "gcc",
     {"c", "cpp"},
     2,
     95,
     "windows",
     "x86_64"},

    {"i686-w64-mingw32-gcc",
     "i686-w64-mingw32-gcc",
     "GNU (mingw-w64)",
     "gcc",
     {"c", "cpp"},
     2,
     90,
     "windows",
     "x86"},

    {"x86_64-w64-mingw32-g++",
     "x86_64-w64-mingw32-g++",
     "GNU (mingw-w64)",
     "gcc",
     {"cpp"},
     1,
     85,
     "windows",
     "x86_64"},

    {"tcc", "tcc", "TinyCC", "gcc", {"c"}, 1, 30, NULL, NULL},

    {"icc", "icc", "Intel", "gcc", {"c", "cpp"}, 2, 40, NULL, NULL},

    {"icx", "icx", "Intel", "gcc", {"c", "cpp"}, 2, 35, NULL, NULL},

    // Microsoft

    {"msvc", "cl", "Microsoft", "msvc", {"c", "cpp"}, 2, 0, NULL, NULL},

    // Zig

    {"zig", "zig", "Zig", "zigcc", {"zig", "c", "cpp"}, 3, 50, NULL, NULL},

    // Rust

    {"rustc", "rustc", "Rust Foundation", "rust", {"rust"}, 1, 50, NULL, NULL},

    // Go

    {"go", "go", "Google", "go", {"go"}, 1, 50, NULL, NULL},

    // Swift

    {"swiftc", "swiftc", "Apple", "swift", {"swift"}, 1, 50, NULL, NULL},

    // Kotlin

    {"kotlinc",
     "kotlinc",
     "JetBrains",
     "kotlin",
     {"kotlin"},
     1,
     50,
     NULL,
     NULL},

    // Java

    {"javac", "javac", "Oracle", "java", {"java"}, 1, 50, NULL, NULL},

    // C#

    {"dotnet",
     "dotnet",
     "Microsoft",
     "dotnet",
     {"csharp", "fsharp", "vb"},
     3,
     50,
     NULL,
     NULL},

    // D

    {"dmd", "dmd", "D Language Foundation", "d", {"d"}, 1, 75, NULL, NULL},

    {"ldc", "ldc2", "LLVM D Compiler", "d", {"d"}, 1, 50, NULL, NULL},

    // Nim

    {"nim", "nim", "Nim", "nim", {"nim"}, 1, 50, NULL, NULL},

    // Haskell

    {"ghc", "ghc", "Haskell", "ghc", {"haskell"}, 1, 50, NULL, NULL},

    // OCaml

    {"ocamlopt", "ocamlopt", "OCaml", "ocaml", {"ocaml"}, 1, 50, NULL, NULL},

    // Fortran

    {"gfortran", "gfortran", "GNU", "gcc", {"fortran"}, 1, 50, NULL, NULL},

    // Pascal

    {"fpc", "fpc", "FreePascal", "pascal", {"pascal"}, 1, 50, NULL, NULL},

    // Ada

    {"gnat", "gnat", "GNU", "gnat", {"ada"}, 1, 50, NULL, NULL},

    // Assembly

    {"nasm", "nasm", "Netwide", "nasm", {"asm"}, 1, 75, NULL, NULL},

    {"yasm", "yasm", "Yasm", "nasm", {"asm"}, 1, 50, NULL, NULL},

    // CUDA

    {"nvcc", "nvcc", "NVIDIA", "cuda", {"cuda", "cpp"}, 2, 50, NULL, NULL},

    // WebAssembly

    {"emcc",
     "emcc",
     "Emscripten",
     "gcc",
     {"c", "cpp", "wasm"},
     3,
     50,
     NULL,
     NULL},

    // JavaScript / TypeScript

    {"node", "node", "Node.js", "node", {"javascript"}, 1, 50, NULL, NULL},

    {"tsc",
     "tsc",
     "Microsoft",
     "typescript",
     {"typescript"},
     1,
     50,
     NULL,
     NULL},

    // V

    {"v", "v", "V Language", "v", {"v"}, 1, 50, NULL, NULL},

    // Crystal

    {"crystal",
     "crystal",
     "Crystal",
     "crystal",
     {"crystal"},
     1,
     50,
     NULL,
     NULL},

    // Julia

    {"julia", "julia", "Julia", "julia", {"julia"}, 1, 50, NULL, NULL},

    // LaTeX

    {"latex", "latex", "TeX", "latex", {"latex"}, 1, 50, NULL, NULL}};

static ArgonObject *create_compiler(ArgonNativeAPI *api, const char *name,
                                    const char *vendor, const char *path,
                                    const char *family, const char **languages,
                                    size_t language_count, int priority,
                                    const char *cross_target,
                                    const char *cross_arch) {

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

  /*
   * "cross_target" is the OS this executable produces binaries for
   * when that's fixed and different from the host.
   *
   * NULL means this compiler targets the host OS.
   */
  api->add_to_hashmap_string_key(
      compiler, "cross_target",
      cross_target == NULL ? api->ARGON_NULL
                           : ARGON_STRING_FROM_C_STRING((char *)cross_target));

  /*
   * "cross_arch" is the architecture this executable produces.
   *
   * NULL means this compiler targets the host architecture.
   */
  api->add_to_hashmap_string_key(
      compiler, "cross_arch",
      cross_arch == NULL ? api->ARGON_NULL
                         : ARGON_STRING_FROM_C_STRING((char *)cross_arch));

  return api->hashmap_to_dictionary(compiler);
}

ARGON_FUNCTION(build_detect_compilers, {
  (void)state;
  (void)argv;

  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  const char *host = host_arch();
  const char *os = host_os();
  ArgonObject *items[64];
  size_t count = 0;
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]);
       i++) { /* * A candidate with cross_target/cross_arch metadata is only *
                 actually "cross" when its target differs from the host. * * For
                 example, on x86_64 Linux: * * x86_64-w64-mingw32-gcc * target:
                 windows/x86_64 * host: linux/x86_64 * -> KEEP * *
                 x86_64-linux-gnu-gcc * target: linux/x86_64 * host:
                 linux/x86_64 * -> skip as redundant/native * *
                 aarch64-linux-gnu-gcc * target: linux/arm64 * host:
                 linux/x86_64 * -> KEEP */
    if (candidates[i].cross_target != NULL ||
        candidates[i].cross_arch != NULL) {
      int same_os = candidates[i].cross_target != NULL && os != NULL &&
                    strcmp(candidates[i].cross_target, os) == 0;
      int same_arch = candidates[i].cross_arch != NULL && host != NULL &&
                      strcmp(candidates[i].cross_arch, host) == 0;
      if (same_os && same_arch)
        continue;
    }
    char path[1024];
    if (!compiler_exists(candidates[i].executable, path, sizeof(path))) {
      continue;
    }
    if (count >= sizeof(items) / sizeof(items[0]))
      break;
    items[count++] =
        create_compiler(api, candidates[i].name, candidates[i].vendor, path,
                        candidates[i].family, candidates[i].languages,
                        candidates[i].language_count, candidates[i].priority,
                        candidates[i].cross_target, candidates[i].cross_arch);
  }

  return api->create_argon_array(items, count);
})

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {

  (void)vm;
  (void)err;

  REGISTER_ARGON_FUNCTION(build_detect_compilers)
}