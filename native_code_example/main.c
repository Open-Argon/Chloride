/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: MIT
 */

#include "../include/Argon.h"
#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>

ArgonObject *i_add(size_t argc, ArgonObject **argv, ArgonError *err,
                   ArgonState *state, ArgonNativeAPI *api) {
  if (argc != 2) {
    return api->throw_argon_error(err, "Runtime Error",
                                  "expected 2 arguments, got %" PRIu64, argc);
  }
  return api->i64_to_argon(api->argon_to_i64(argv[0], err) +
                           api->argon_to_i64(argv[1], err));
}

ArgonObject *hello_world(size_t argc, ArgonObject **argv, ArgonError *err,
                         ArgonState *state, ArgonNativeAPI *api) {
  printf("hello world from native code\n");
  return api->ARGON_NULL;
}

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api,
                       ArgonObjectRegister *reg) {
  api->register_ArgonObject(
      reg, "hello_world",
      api->create_argon_native_function("hello_world", hello_world));
  api->register_ArgonObject(
      reg, "i_add",
      api->create_argon_native_function("i_add", i_add));
}