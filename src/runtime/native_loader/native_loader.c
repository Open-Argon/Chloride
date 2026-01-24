/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "native_loader.h"
#include "../../err.h"
#include "../objects/dictionary/dictionary.h"
#include "../objects/literals/literals.h"
#include "../objects/string/string.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// load_native_code(path to dll / so / dylib)
ArgonObject *ARGON_LOAD_NATIVE_CODE(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  if (argc != 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "load_native_code expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  ArgonObject *path = argv[0];
  char *path_c = argon_string_to_c_string_malloc(path);

#if defined(_WIN32) || defined(_WIN64)
  HMODULE handle = LoadLibraryA(path_c);
  if (!handle) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "Unable to load native code at path: %s", path_c);
    free(path_c);
    return ARGON_NULL;
  }

  FARPROC proc = GetProcAddress(
          handle, "argon_module_init");

  if (!proc) {
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "Unable to find argon_module_init in the native code at path: %s",
        path_c);
    free(path_c);
    FreeLibrary(handle);
    return ARGON_NULL;
  }
  argon_module_init_fn init = (argon_module_init_fn)(void *)proc;
#else
  void *handle = dlopen(path_c, RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    const char *dlerr = dlerror();
    *err = create_err(0, 0, 0, "", "Runtime Error", "%s",
                      dlerr ? dlerr : "unknown");
    free(path_c);
    return ARGON_NULL;
  }

  argon_module_init_fn init =
      (argon_module_init_fn)dlsym(
          handle, "argon_module_init");

  if (!init) {
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "Unable to find argon_module_init in the native code at path: %s",
        path_c);
    free(path_c);
    dlclose(handle);
    return ARGON_NULL;
  }
#endif

  hashmap_GC *reg = createHashmap_GC();

  init(state, api, err, reg);

  free(path_c);

  if (err->exists)
    return ARGON_NULL;

  return create_dictionary(reg);
}