/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "signals.h"
#include "../object.h"
#include "../string/string.h"

ArgonObject*SIGNAL_CLASS;
ArgonObject*END_ITERATION;

void init_signals() {
  SIGNAL_CLASS = new_class();
  add_builtin_field(SIGNAL_CLASS, __name__,
                    new_string_object_null_terminated("signal"));

  END_ITERATION = new_small_instance(SIGNAL_CLASS, 0);
  add_builtin_field(END_ITERATION, __name__,
                    new_string_object_null_terminated("end_iteration"));
}