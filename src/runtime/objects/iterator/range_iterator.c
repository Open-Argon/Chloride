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
#include <stdint.h>

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
  if (argc < 2 || argc > 4) {
    *err =
        create_err(state->source_location.line, state->source_location.column,
                   state->source_location.length, state->path, "Runtime Error",
                   "__init__ expects 2 to 4 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  if (argc == 2) {
    if (argv[1]->type == TYPE_NUMBER && argv[1]->value.as_number->is_int64) {
      self->value.as_range_iterator->is_int64 = true;
      self->value.as_range_iterator->stop.i64 = argv[1]->value.as_number->n.i64;
      self->value.as_range_iterator->current.i64 = 0;
      self->value.as_range_iterator->step.i64 = 1;
    } else {
      self->value.as_range_iterator->is_int64 = false;
      self->value.as_range_iterator->stop.obj = argv[1];
      self->value.as_range_iterator->current.obj = &small_ints[-small_ints_min];
      self->value.as_range_iterator->step.obj =
          &small_ints[-small_ints_min + 1];
    }
  } else {
    if ((argv[1]->type == TYPE_NUMBER && argv[1]->value.as_number->is_int64) &&
        (argv[2]->type == TYPE_NUMBER && argv[2]->value.as_number->is_int64) &&
        (argc != 4 || (argv[3]->type == TYPE_NUMBER &&
                       argv[3]->value.as_number->is_int64))) {
      self->value.as_range_iterator->is_int64 = true;
      self->value.as_range_iterator->current.i64 =
          argv[1]->value.as_number->n.i64;
      self->value.as_range_iterator->stop.i64 = argv[2]->value.as_number->n.i64;
      if (argc == 4) {
        self->value.as_range_iterator->step.i64 =
            argv[3]->value.as_number->n.i64;
        if (self->value.as_range_iterator->step.i64 == 0) {
          *err = create_err(state->source_location.line,
                            state->source_location.column,
                            state->source_location.length, state->path,
                            "Runtime Error", "step cannot be 0");
        }
      }
    } else {
      self->value.as_range_iterator->is_int64 = false;
      self->value.as_range_iterator->current.obj = argv[1];
      self->value.as_range_iterator->stop.obj = argv[2];
      if (argc == 4) {
        self->value.as_range_iterator->step.obj = argv[3];
      }
    }
  }
  return ARGON_NULL;
}

ArgonObject *ARGON_RANGE_ITERATOR_TYPE___iter__(size_t argc, ArgonObject **argv,
                                                ArErr *err, RuntimeState *state,
                                                ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err =
        create_err(state->source_location.line, state->source_location.column,
                   state->source_location.length, state->path, "Runtime Error",
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
    *err =
        create_err(state->source_location.line, state->source_location.column,
                   state->source_location.length, state->path, "Runtime Error",
                   "__next__ expects 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  struct as_range_iterator *range_iterator = self->value.as_range_iterator;
  if (range_iterator->is_int64) {
    int64_t current_val = range_iterator->current.i64;
    int64_t stop_val = range_iterator->stop.i64;
    int64_t step_val = range_iterator->step.i64;

    if ((step_val>0 && current_val>=stop_val) || (step_val<0 && current_val<=stop_val)) {
      return END_ITERATION;
    }
    range_iterator->current.i64 = current_val+step_val;
    return new_number_object_from_int64(current_val);
  } else {
    ArgonObject *current_val = range_iterator->current.obj;
    ArgonObject *stop_val = range_iterator->stop.obj;
    ArgonObject *step_val = range_iterator->step.obj;

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

    range_iterator->current.obj = ARGON_NUMBER_TYPE___add__(
        2, (ArgonObject *[]){current_val, step_val}, err, state, api);

    return current_val;
  }
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