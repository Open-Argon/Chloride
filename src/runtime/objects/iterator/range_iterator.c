/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "range_iterator.h"
#include "../../../err.h"
#include "../functions/functions.h"
#include "../literals/literals.h"
#include "../number/number.h"
#include "../signals/signals.h"
#include "../string/string.h"
#include <inttypes.h>

ArgonObject *ARGON_RANGE_ITERATOR_TYPE;

ArgonObject *ARGON_RANGE_ITERATOR_TYPE___new__(size_t argc, ArgonObject **argv,
                                               ArErr *err, RuntimeState *state,
                                               ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc < 1) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__new__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *new_obj =
      new_instance(argv[0], sizeof(struct as_range_iterator));
  new_obj->value.as_range_iterator =
      (struct as_range_iterator *)((char *)new_obj + sizeof(ArgonObject));
  return new_obj;
}

ArgonObject *ARGON_RANGE_ITERATOR_TYPE___init__(size_t argc, ArgonObject **argv,
                                                ArErr *err, RuntimeState *state,
                                                ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 4) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__init__ expects 4 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  self->value.as_range_iterator->current = argv[1];
  self->value.as_range_iterator->stop = argv[2];
  self->value.as_range_iterator->step = argv[3];
  return ARGON_NULL;
}

ArgonObject *ARGON_RANGE_ITERATOR_TYPE___iter__(size_t argc, ArgonObject **argv,
                                                ArErr *err, RuntimeState *state,
                                                ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__iter__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  return self;
}

ArgonObject *ARGON_RANGE_ITERATOR_TYPE___next__(size_t argc, ArgonObject **argv,
                                                ArErr *err, RuntimeState *state,
                                                ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__next__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  struct as_range_iterator *range_iterator = self->value.as_range_iterator;
  ArgonObject *current_val = range_iterator->current;
  ArgonObject *stop_val = range_iterator->stop;
  ArgonObject *step_val = range_iterator->step;

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

  range_iterator->current = ARGON_NUMBER_TYPE___add__(
      2, (ArgonObject *[]){current_val, step_val}, err, state, api);

  return current_val;
}

void init_range_iterator() {
  ARGON_RANGE_ITERATOR_TYPE = new_class();
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __name__,
                    new_string_object_null_terminated("Iterator"));
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __new__,
                    create_argon_native_function(
                        "__new__", ARGON_RANGE_ITERATOR_TYPE___new__));
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __init__,
                    create_argon_native_function(
                        "__init__", ARGON_RANGE_ITERATOR_TYPE___init__));
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __iter__,
                    create_argon_native_function(
                        "__iter__", ARGON_RANGE_ITERATOR_TYPE___iter__));
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __next__,
                    create_argon_native_function(
                        "__next__", ARGON_RANGE_ITERATOR_TYPE___next__));
}