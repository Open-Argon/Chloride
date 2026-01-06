/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "native_loader.h"
#include "../../err.h"
#include "../objects/literals/literals.h"
#include "../objects/dictionary/dictionary.h"
#include "../objects/string/string.h"
#include <dlfcn.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

// load_native_code(path to dll / so)
ArgonObject *ARGON_LOAD_NATIVE_CODE(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api) {
  if (argc != 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "load_native_code expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }

  ArgonObject *path = argv[0];
  char *path_c = argon_string_to_c_string_malloc(path, err, state);

  void *handle = dlopen(path_c, RTLD_NOW | RTLD_LOCAL);

  if (!handle) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "Unable to load native code at path: %s", path_c);
    free(path_c);
    return ARGON_NULL;
  }

  void (*init)(RuntimeState *, ArgonNativeAPI *, hashmap_GC *) =
      dlsym(handle, "argon_module_init");

  if (!init) {
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "Unable find argon_module_init in the native code at path: %s", path_c);
    free(path_c);
    dlclose(handle);
    return ARGON_NULL;
  }

  hashmap_GC* reg = createHashmap_GC();

  init(state, api, reg);

  free(path_c);

  return create_dictionary(reg);
}