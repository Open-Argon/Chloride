// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Argon.h"
#include "random.h"

ArgonObject *argon_random_os_seed(size_t argc, ArgonObject **argv, ArgonError *err,
                               ArgonState *state, ArgonNativeAPI *api) {
  (void)state;
  (void)argv;
  if (api->fix_to_arg_size(0, argc, err))
    return api->ARGON_NULL;

  return api->i64_to_argon(random_os_seed());
}

ArgonObject *argon_random_seed(size_t argc, ArgonObject **argv, ArgonError *err,
                               ArgonState *state, ArgonNativeAPI *api) {
  (void)state;
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  int64_t seed = api->argon_to_i64(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  ArgonObject *random = api->create_argon_buffer(sizeof(Random));
  if (api->is_error(err))
    return api->ARGON_NULL;

  struct buffer random_buffer = api->argon_buffer_to_buffer(random, err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  random_seed(random_buffer.data, seed);

  return random;
}

ArgonObject *argon_random_next_number(size_t argc, ArgonObject **argv, ArgonError *err,
                               ArgonState *state, ArgonNativeAPI *api) {
  (void)state;
  if (api->fix_to_arg_size(1, argc, err))
    return api->ARGON_NULL;

  struct buffer random_buffer = api->argon_buffer_to_buffer(argv[0], err);
  if (api->is_error(err))
    return api->ARGON_NULL;

  return api->double_to_argon(random_next_double(random_buffer.data));
}

void argon_module_init(ArgonState *vm, ArgonNativeAPI *api, ArgonError *err,
                       ArgonObjectRegister *reg) {
  (void)vm;
  (void)err;
  api->register_ArgonObject(
      reg, "random_os_seed",
      api->create_argon_native_function("random_os_seed", argon_random_os_seed));
  api->register_ArgonObject(
      reg, "random_seed",
      api->create_argon_native_function("random_seed", argon_random_seed));
  api->register_ArgonObject(
      reg, "random_next_number",
      api->create_argon_native_function("random_next_number", argon_random_next_number));
}