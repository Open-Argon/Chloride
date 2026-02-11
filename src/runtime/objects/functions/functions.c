/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../object.h"
#include "../string/string.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

ArgonObject *ARGON_FUNCTION_TYPE = NULL;

ArgonObject *create_argon_native_function(char *name, native_fn native_fn) {
  ArgonObject *object = new_instance(ARGON_FUNCTION_TYPE,0);
  object->type = TYPE_NATIVE_FUNCTION;
  add_builtin_field(object, __name__,
                    new_string_object(name, strlen(name), 0, 0));
  object->value.native_fn = native_fn;
  return object;
}