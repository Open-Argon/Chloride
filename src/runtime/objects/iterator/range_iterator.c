/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "range_iterator.h"
#include "../../../err.h"
#include "../exceptions/exceptions.h"

#include "../literals/literals.h"
#include "../number/number.h"
#include "../string/string.h"
#include <inttypes.h>
#include <stdint.h>

ArgonObject *ARGON_RANGE_ITERATOR_TYPE;

ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __new__, {

  (void)api;
  (void)state;
  if (argc < 1) {
    *err =
        create_err(RuntimeError,
                   "__new__ expects at least 1 argument, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *new_obj =
      new_instance(argv[0], sizeof(struct as_range_iterator));
  new_obj->type = TYPE_RANGE_ITERATOR;
  new_obj->value.as_range_iterator =
      (struct as_range_iterator *)((char *)new_obj + sizeof(ArgonObject));
  return new_obj;
})

ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __init__, {

  (void)api;
  (void)state;
  if (argc < 2 || argc > 4) {
    *err = create_err(RuntimeError,
                      "__init__ expects 2 to 4 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  for (uint64_t i = 1; i < argc; i++) {
    if (argv[i]->type != TYPE_NUMBER) {
      *err = create_err(RuntimeError, "expects number");
      return ARGON_NULL;
    }
  }
  ArgonObject *self = argv[0];
  self->value.as_range_iterator->inclusive = false;
  if (argc == 2) {
    if (argv[1]->type == TYPE_NUMBER && argv[1]->value.as_number->is_int64) {
      self->value.as_range_iterator->is_int64 = true;
      self->value.as_range_iterator->stop.i64 = argv[1]->value.as_number->n.i64;
      self->value.as_range_iterator->current.i64 = 0;
      self->value.as_range_iterator->step.i64 = 1;
    } else {
      self->value.as_range_iterator->is_int64 = false;
      self->value.as_range_iterator->stop.obj = argv[1];
      self->value.as_range_iterator->current.obj = &small_ints[-small_ints_min].obj;
      self->value.as_range_iterator->step.obj =
          &small_ints[-small_ints_min + 1].obj;
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
      self->value.as_range_iterator->step.i64 = 1;
      if (argc == 4) {
        self->value.as_range_iterator->step.i64 =
            argv[3]->value.as_number->n.i64;
        if (self->value.as_range_iterator->step.i64 == 0) {
          *err = create_err(RuntimeError, "step cannot be 0");
          return ARGON_NULL;
        }
      }
    } else {
      self->value.as_range_iterator->is_int64 = false;
      self->value.as_range_iterator->current.obj = argv[1];
      self->value.as_range_iterator->stop.obj = argv[2];
      self->value.as_range_iterator->step.obj =
          &small_ints[-small_ints_min + 1].obj;
      if (argc == 4) {
        self->value.as_range_iterator->step.obj = argv[3];
      }
    }
  }
  return ARGON_NULL;
})

ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __iter__, {

  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__iter__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *new_obj =
      new_instance(ARGON_RANGE_ITERATOR_TYPE, sizeof(struct as_range_iterator));
  new_obj->type = TYPE_RANGE_ITERATOR;
  new_obj->value.as_range_iterator =
      (struct as_range_iterator *)((char *)new_obj + sizeof(ArgonObject));

  *new_obj->value.as_range_iterator = *self->value.as_range_iterator;
  return new_obj;
})

ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __next__, {

  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(RuntimeError, "__next__ expects 1 argument, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  struct as_range_iterator *range_iterator = self->value.as_range_iterator;
  if (range_iterator->is_int64) {
    int64_t current_val = range_iterator->current.i64;
    int64_t stop_val = range_iterator->stop.i64;
    int64_t step_val = range_iterator->step.i64;

    if (range_iterator->inclusive
            ? (step_val > 0 && current_val > stop_val) ||
                  (step_val < 0 && current_val < stop_val)
            : (step_val > 0 && current_val >= stop_val) ||
                  (step_val < 0 && current_val <= stop_val)) {
      err->ptr = StopIteration_instance;
      return ARGON_NULL;
    }
    range_iterator->current.i64 = current_val + step_val;
    return new_number_object_from_int64(current_val);
  } else {
    ArgonObject *current_val = range_iterator->current.obj;
    ArgonObject *stop_val = range_iterator->stop.obj;
    ArgonObject *step_val = range_iterator->step.obj;

    if ((ARGON_NUMBER_TYPE___greater_than__(
             2, (ArgonObject *[]){step_val, &small_ints[-small_ints_min].obj}, NULL, err,
             state, api) == ARGON_TRUE &&
         ARGON_NUMBER_TYPE___greater_than_equal__(
             2, (ArgonObject *[]){current_val, stop_val}, NULL, err, state, api) ==
             ARGON_TRUE) ||
        ((ARGON_NUMBER_TYPE___less_than__(
              2, (ArgonObject *[]){step_val, &small_ints[-small_ints_min].obj}, NULL, err,
              state, api) == ARGON_TRUE &&
          ARGON_NUMBER_TYPE___less_than_equal__(
              2, (ArgonObject *[]){current_val, stop_val}, NULL, err, state, api) ==
              ARGON_TRUE))) {
      err->ptr = StopIteration_instance;
      return ARGON_NULL;
    }

    range_iterator->current.obj = ARGON_NUMBER_TYPE___add__(
        2, (ArgonObject *[]){current_val, step_val}, NULL, err, state, api);

    return current_val;
  }
})

void init_range_iterator() {
  ARGON_RANGE_ITERATOR_TYPE = new_class();
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __name__,
                    new_string_object_null_terminated("Range"));
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __new__)
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __init__)
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __iter__)
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __next__)
}