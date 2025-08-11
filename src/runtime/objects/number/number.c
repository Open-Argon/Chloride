/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "number.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stdio.h>
#include "../functions/functions.h"

ArgonObject *ARGON_NUMBER_TYPE;

ArgonObject *ARGON_NUMBER_TYPE___string__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 arguments, got %" PRIu64, argc);
  }
  double val = mpq_get_d(argv[0]->value.as_number);
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%.15g", val);
  return new_string_object_null_terminated(buffer);
}

void create_ARGON_NUMBER_TYPE() {
  ARGON_NUMBER_TYPE = new_object();
  add_field(ARGON_NUMBER_TYPE, "__name__",
            new_string_object_null_terminated("number"));
  add_field(ARGON_NUMBER_TYPE, "__string__",
            create_argon_native_function("__string__", ARGON_NUMBER_TYPE___string__));
}

ArgonObject *new_number_object(char *data) {
  ArgonObject *object = new_object();
  add_field(object, "__class__", ARGON_NUMBER_TYPE);
  object->type = TYPE_NUMBER;
  mpq_init(object->value.as_number);
  mpq_set_str(object->value.as_number, data, 62);
  return object;
}