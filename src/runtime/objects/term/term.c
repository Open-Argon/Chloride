/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "term.h"
#include "../../call/call.h"
#include <stddef.h>
#include <stdio.h>

ArgonObject *term_log(size_t argc, ArgonObject **argv, ArErr *err,
                      RuntimeState *state, ArgonNativeAPI *api) {
  (void)api;
  for (size_t i = 0; i < argc; i++) {
    if (i != 0)
      printf(" ");
    ArgonObject *string_convert_method = get_builtin_field_for_class(
        get_builtin_field(argv[i], __class__), __string__, argv[i]);

    if (string_convert_method) {
      ArgonObject *string_object =
          argon_call(string_convert_method, 0, NULL, err, state);
      fwrite(string_object->value.as_str->data, sizeof(char),
             string_object->value.as_str->length, stdout);
    }
  }
  printf("\n");
  return ARGON_NULL;
}