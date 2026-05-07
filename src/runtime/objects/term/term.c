/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "term.h"
#include "../../../shell.h"
#include "../../call/call.h"
#include "../../objects/literals/literals.h"
#include "../exceptions/exceptions.h"
#include "../../../err.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

ARGON_METHOD(term, log, {
  (void)api;
  for (size_t i = 0; i < argc; i++) {
    if (i != 0)
      printf(" ");
    ArgonObject *string_convert_method = get_builtin_field_for_class(
        get_builtin_field(argv[i], __class__), __string__, argv[i]);

    if (string_convert_method) {
      ArgonObject *string_object =
          argon_call(string_convert_method, 0, NULL, NULL, err, state);
      struct string string = api->argon_to_string(string_object, err);
      if (api->is_error(err))
        return ARGON_NULL;
      fwrite(string.data, sizeof(char), string.length, stdout);
    }
  }
  printf("\n");
  return ARGON_NULL;
})

ARGON_METHOD(term, error, {
  if (argc != 1)
    return api->throw_argon_error(err, api->RuntimeError,
                           "expects 1 argument, got %" PRIu64, argc);

  if (!is_instance(argv[0], BaseException))
    return api->throw_argon_error(err, api->RuntimeError,
                           "expects instance of BaseException");

  ArErr err = {argv[0]};

  output_err(&err);

  return ARGON_NULL;
})

ARGON_METHOD(term, input, {
  if (argc >= 2)
    return api->throw_argon_error(err, api->RuntimeError,
                           "expects at most 1 argument, got %" PRIu64, argc);
  struct string prompt_str = {NULL, 0};
  if (argc == 1) {
    ArgonObject *string_convert_method = get_builtin_field_for_class(
        get_builtin_field(argv[0], __class__), __string__, argv[0]);

    if (string_convert_method) {
      ArgonObject *string_object =
          argon_call(string_convert_method, 0, NULL, NULL, err, state);
      prompt_str = api->argon_to_string(string_object, err);
      if (api->is_error(err))
        return ARGON_NULL;
    }
  }

  char *prompt = malloc(prompt_str.length + 1);
  if (!prompt)
    return api->throw_argon_error(err, api->RuntimeError, "out of memory");

  memcpy(prompt, prompt_str.data, prompt_str.length);
  prompt[prompt_str.length] = '\0';

  char *user_input = input(prompt);
  free(prompt);
  if (!user_input)
    return api->string_to_argon((struct string){NULL, 0});

  ArgonObject *result =
      api->string_to_argon((struct string){user_input, strlen(user_input)});
  free(user_input);

  return result;
})