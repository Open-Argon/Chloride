/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../object.h"
#include <stdint.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "string.h"

ArgonObject *ARGON_STRING_TYPE = NULL;

ArgonObject *new_string_object(char*data, size_t length) {
  ArgonObject * object = new_object();
  add_field(object, "__class__", ARGON_STRING_TYPE);
  object->type = TYPE_STRING;
  object->value.as_str.data = data;
  object->value.as_str.length = length;
  return object;
}

ArgonObject *new_string_object_null_terminated(char*data) {
  return new_string_object(data, strlen(data));
}