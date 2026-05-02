/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "term.h"
#include "../../../shell.h"
#include "../../call/call.h"
#include "../../objects/literals/literals.h"
#include <stddef.h>
#include <stdio.h>

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

ARGON_METHOD(term, input, {
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
  fflush(stdout);

  char *user_input = input("");
  
  return api->string_to_argon((struct string){user_input, strlen(user_input)});
})