/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "range_iterator.h"
#include "../../../err.h"
#include "../literals/literals.h"
#include "../number/number.h"
#include "../string/string.h"
#include "../signals/signals.h"
#include "../functions/functions.h"
#include <inttypes.h>

ArgonObject *ARGON_RANGE_ITERATOR_TYPE;

ArgonObject *create_ARGON_RANGE_ITERATOR_TYPE___next__(size_t argc,
                                                       ArgonObject **argv,
                                                       ArErr *err,
                                                       RuntimeState *state,
                                                       ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__next__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *current_val = get_builtin_field(self, current);
  ArgonObject *stop_val = get_builtin_field(self, stop);
  ArgonObject *step_val = get_builtin_field(self, step);

  if ((ARGON_NUMBER_TYPE___greater_than__(
           2, (ArgonObject *[]){step_val, &small_ints[-small_ints_min]}, err,
           state, api) == ARGON_TRUE &&
       ARGON_NUMBER_TYPE___greater_than_equal__(
           2, (ArgonObject *[]){current_val, stop_val}, err, state, api) ==
           ARGON_TRUE) ||
      ((ARGON_NUMBER_TYPE___less_than__(
            2, (ArgonObject *[]){step_val, &small_ints[-small_ints_min]}, err,
            state, api) == ARGON_TRUE &&
        ARGON_NUMBER_TYPE___less_than_equal__(
            2, (ArgonObject *[]){current_val, stop_val}, err, state, api) ==
            ARGON_TRUE))) {
              return END_ITERATION;
  }

  add_builtin_field(self, current, ARGON_NUMBER_TYPE___add__(
           2, (ArgonObject *[]){current_val, step_val}, err,
           state, api));

  return current_val;
}

void init_range_iterator() {
  ARGON_RANGE_ITERATOR_TYPE = new_class();
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __name__,
                    new_string_object_null_terminated("Iterator"));
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __next__,
                    create_argon_native_function("__next__", create_ARGON_RANGE_ITERATOR_TYPE___next__));
}