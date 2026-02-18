/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: MIT
 */

#include "../include/Argon.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

ArgonObject *int_add(size_t argc, ArgonObject **argv, ArgonError *err,
                     ArgonState *state, ArgonNativeAPI *api) {
  if (argc != 2) {
    return api->throw_argon_error(err, "Runtime Error",
                                  "expected 2 arguments, got %" PRIu64, argc);
  }
  int64_t a = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  int64_t b = api->argon_to_i64(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  return api->i64_to_argon(a + b);
}

ArgonObject *double_add(size_t argc, ArgonObject **argv, ArgonError *err,
                        ArgonState *state, ArgonNativeAPI *api) {
  if (argc != 2) {
    return api->throw_argon_error(err, "Runtime Error",
                                  "expected 2 arguments, got %" PRIu64, argc);
  }
  double a = api->argon_to_double(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  double b = api->argon_to_double(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  return api->double_to_argon(a + b);
}

ArgonObject *rational_add(size_t argc, ArgonObject **argv, ArgonError *err,
                          ArgonState *state, ArgonNativeAPI *api) {
  if (argc != 2) {
    return api->throw_argon_error(err, "Runtime Error",
                                  "expected 2 arguments, got %" PRIu64, argc);
  }
  struct rational a = api->argon_to_rational(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  struct rational b = api->argon_to_rational(argv[1], err);
  if (api->is_error(err))
    return api->ARGON_NULL;
  return api->rational_to_argon(
      (struct rational){a.n * b.d + b.n * a.d, a.d * b.d});
}

ArgonObject *hello_world(size_t argc, ArgonObject **argv, ArgonError *err,
                         ArgonState *state, ArgonNativeAPI *api) {
  printf("hello world from native code\n");
  return api->ARGON_NULL;
}

ArgonObject *yooo(size_t argc, ArgonObject **argv, ArgonError *err,
                  ArgonState *state, ArgonNativeAPI *api) {
  if (api->fix_to_arg_size(1, argc, err)) {
    return api->ARGON_NULL;
  }

  struct string name = api->argon_to_string(argv[0], err);

  if (api->is_error(err))
    return api->ARGON_NULL;

  const char *prefix = "Yooo ";
  const char *suffix = ", how are you?";

  const uint64_t length = strlen(prefix) + strlen(suffix) + name.length;

  char *data = malloc(length);

  memcpy(data, prefix, strlen(prefix));
  memcpy(data + strlen(prefix), name.data, name.length);
  memcpy(data + strlen(prefix) + name.length, suffix, strlen(suffix));
  ArgonObject*output = api->string_to_argon((struct string){data, length});
  free(data);
  return output;
}

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError*err,
                  ArgonObjectRegister *reg) {
  api->register_ArgonObject(
      reg, "hello_world",
      api->create_argon_native_function("hello_world", hello_world));

  api->register_ArgonObject(
      reg, "int_add", api->create_argon_native_function("int_add", int_add));
  api->register_ArgonObject(
      reg, "double_add",
      api->create_argon_native_function("double_add", double_add));
  api->register_ArgonObject(
      reg, "rational_add",
      api->create_argon_native_function("rational_add", rational_add));
  api->register_ArgonObject(reg, "yooo",
                            api->create_argon_native_function("yooo", yooo));
  api->register_ArgonObject(
      reg, "nice", api->string_to_argon((struct string){"hello world", 11}));
}