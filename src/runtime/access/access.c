/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "access.h"
#include <stdio.h>

ArgonObject *ARGON_TYPE_TYPE___get_attr__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 3) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__get_attr__ expects 3 arguments, got %" PRIu64);
    return ARGON_NULL;
  }
  ArgonObject *to_access = argv[0];
  bool check_field = argv[1] == ARGON_TRUE;
  if (check_field) {
    ArgonObject *access = argv[2];
    ArgonObject *value = get_field_l(to_access, access->value.as_str.data,
                                     access->value.as_str.length, true, false);
    if (value)
      return value;
    ArgonObject *name = get_field_for_class(
        get_field(to_access, "__class__", false, false), "__name__", to_access);
    *err = create_err(
        0, 0, 0, "", "Runtime Error", "'%.*s' object has no attribute '%.*s'",
        (int)name->value.as_str.length, name->value.as_str.data,
        (int)access->value.as_str.length, access->value.as_str.data);
  }
  return ARGON_NULL;
}