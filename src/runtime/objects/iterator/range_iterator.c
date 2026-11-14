/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
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

// Copies a number object's value into an initialised mpq_t.
static void number_to_mpq(ArgonObject *number, mpq_t out) {
  if (number->value.as_number->is_int64)
    mpq_set_si64(out, number->value.as_number->n.i64, 1);
  else
    mpq_set(out, *number->value.as_number->n.mpq);
}

// Reads the current, stop and step values of a range as rationals.
static void range_to_mpq(struct as_range_iterator *range, mpq_t current,
                         mpq_t stop, mpq_t step) {
  if (range->is_int64) {
    mpq_set_si64(current, range->current.i64, 1);
    mpq_set_si64(stop, range->stop.i64, 1);
    mpq_set_si64(step, range->step.i64, 1);
  } else {
    number_to_mpq(range->current.obj, current);
    number_to_mpq(range->stop.obj, stop);
    number_to_mpq(range->step.obj, step);
  }
}

// Whether a value that compares to stop as `compared_to_stop` (<0, 0, >0) is
// beyond the end of a range moving in the direction of step_sign.
static bool past_end(int compared_to_stop, int step_sign, bool inclusive) {
  if (step_sign > 0)
    return inclusive ? compared_to_stop > 0 : compared_to_stop >= 0;
  return inclusive ? compared_to_stop < 0 : compared_to_stop <= 0;
}

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
  if (argc == 4 && argv[3]->value.as_number->is_int64 &&
      argv[3]->value.as_number->n.i64 == 0) {
    *err = create_err(RuntimeError, "step cannot be 0");
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  struct as_range_iterator *r = self->value.as_range_iterator;
  // Re-initialising: let go of any number objects from before.
  if (!r->is_int64) {
    gc_decref(r->current.obj);
    gc_decref(r->stop.obj);
    gc_decref(r->step.obj);
  }
  // (cleared either way: in int64 mode these hold integers, not pointers)
  r->current.obj = r->stop.obj = r->step.obj = NULL;
  r->is_int64 = false;
  self->value.as_range_iterator->inclusive = false;
  if (argc == 2) {
    if (argv[1]->type == TYPE_NUMBER && argv[1]->value.as_number->is_int64) {
      self->value.as_range_iterator->is_int64 = true;
      self->value.as_range_iterator->stop.i64 = argv[1]->value.as_number->n.i64;
      self->value.as_range_iterator->current.i64 = 0;
      self->value.as_range_iterator->step.i64 = 1;
    } else {
      self->value.as_range_iterator->is_int64 = false;
      gc_assign(&r->stop.obj, argv[1]);
      gc_assign(&r->current.obj, &small_ints[-small_ints_min].obj);
      gc_assign(&r->step.obj, &small_ints[-small_ints_min + 1].obj);
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
      gc_assign(&r->current.obj, argv[1]);
      gc_assign(&r->stop.obj, argv[2]);
      gc_assign(&r->step.obj, argc == 4 ? argv[3]
                                        : &small_ints[-small_ints_min + 1].obj);
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
  struct as_range_iterator *copy = new_obj->value.as_range_iterator;
  if (!copy->is_int64) { // the copy holds its own references
    gc_incref(copy->current.obj);
    gc_incref(copy->stop.obj);
    gc_incref(copy->step.obj);
  }
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
    ArgonObject *step_val = range_iterator->step.obj;

    mpq_t current, stop, step;
    mpq_inits(current, stop, step, NULL);
    range_to_mpq(range_iterator, current, stop, step);
    bool finished = past_end(mpq_cmp(current, stop), mpq_sgn(step),
                             range_iterator->inclusive);
    mpq_clears(current, stop, step, NULL);
    if (finished) {
      err->ptr = StopIteration_instance;
      return ARGON_NULL;
    }

    gc_assign(&range_iterator->current.obj,
              ARGON_FUNC_ARGON_NUMBER_TYPE___add__(
                  2, (ArgonObject *[]){current_val, step_val}, NULL, err, state,
                  api));

    return current_val;
  }
})

// `value in range`: whether iterating the range would produce value.
ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __contains__, {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(RuntimeError,
                      "__contains__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER)
    return ARGON_FALSE;
  struct as_range_iterator *range = argv[0]->value.as_range_iterator;

  mpq_t start, stop, step, value;
  mpq_inits(start, stop, step, value, NULL);
  range_to_mpq(range, start, stop, step);
  number_to_mpq(argv[1], value);

  int step_sign = mpq_sgn(step);
  int compared_to_start = mpq_cmp(value, start);
  bool contained =
      step_sign != 0 &&
      (step_sign > 0 ? compared_to_start >= 0 : compared_to_start <= 0) &&
      !past_end(mpq_cmp(value, stop), step_sign, range->inclusive);

  if (contained) {
    // value must be a whole number of steps from the start
    mpq_sub(value, value, start);
    mpq_div(value, value, step);
    contained = mpz_cmp_ui(mpq_denref(value), 1) == 0;
  }

  mpq_clears(start, stop, step, value, NULL);
  return contained ? ARGON_TRUE : ARGON_FALSE;
})

void init_range_iterator() {
  ARGON_RANGE_ITERATOR_TYPE = new_class();
  add_builtin_field(ARGON_RANGE_ITERATOR_TYPE, __name__,
                    new_string_object_null_terminated("Range"));
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __new__)
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __init__)
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __iter__)
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __next__)
  MOUNT_ARGON_METHOD(ARGON_RANGE_ITERATOR_TYPE, __contains__)
}