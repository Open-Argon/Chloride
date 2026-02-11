/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "number.h"
#include "../../../err.h"
#include "../../../memory.h"
#include "../functions/functions.h"
#include "../string/string.h"
#include <gmp.h>
#include <inttypes.h>
#include <mpfr.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

ArgonObject *ARGON_NUMBER_TYPE;

#include "../../call/call.h"
#include "../literals/literals.h"
#include <gmp.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIGNIFICANT_DIGITS 16

void mpq_fdiv(mpq_t result, const mpq_t a, const mpq_t b) {
  mpq_t tmp;
  mpq_init(tmp);

  // tmp = a / b
  mpq_div(tmp, a, b);
  mpq_canonicalize(tmp);

  // floor(tmp) = floor(num / den)
  mpz_t q;
  mpz_init(q);

  mpz_fdiv_q(q, mpq_numref(tmp), mpq_denref(tmp));

  // result = q / 1
  mpq_set_z(result, q);

  mpz_clear(q);
  mpq_clear(tmp);
}

void mpq_fmod(mpq_t result, const mpq_t a, const mpq_t b) {
  // result = a - b * floor(a / b)

  mpq_t q, tmp;
  mpq_init(q);
  mpq_init(tmp);

  // q = floor(a / b)   (integer-valued mpq)
  mpq_fdiv(q, a, b);

  // tmp = b * q
  mpq_mul(tmp, b, q);

  // result = a - tmp
  mpq_sub(result, a, tmp);

  mpq_clear(q);
  mpq_clear(tmp);
}

ArgonObject *ARGON_NUMBER_TYPE___new__(size_t argc, ArgonObject **argv,
                                       ArErr *err, RuntimeState *state,
                                       ArgonNativeAPI *api) {
  (void)api;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__new__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *object = argv[1];

  self->type = TYPE_NUMBER;
  ArgonObject *boolean_convert_method = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __number__, object);
  if (boolean_convert_method) {
    ArgonObject *boolean_object =
        argon_call(boolean_convert_method, 0, NULL, err, state);
    if (err->exists)
      return ARGON_NULL;
    return boolean_object;
  }
  ArgonObject *type_name = get_builtin_field_for_class(
      get_builtin_field(object, __class__), __name__, object);
  *err = create_err(
      0, 0, 0, "", "Runtime Error", "cannot convert type '%.*s' to number",
      type_name->value.as_str->length, type_name->value.as_str->data);
  return ARGON_NULL;
}

ArgonObject *ARGON_NUMBER_TYPE___number__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__number__ expects 1 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return argv[0];
}

ArgonObject *ARGON_NUMBER_TYPE___boolean__(size_t argc, ArgonObject **argv,
                                           ArErr *err, RuntimeState *state,
                                           ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return argv[0]->as_bool ? ARGON_TRUE : ARGON_FALSE;
}

ArgonObject *ARGON_NUMBER_TYPE___negation__(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state,
                                            ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__negation__ expects 1 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[0]->value.as_number->is_int64) {
    return new_number_object_from_int64(-argv[0]->value.as_number->n.i64);
  }
  mpq_t result;
  mpq_init(result);
  mpq_neg(result, *argv[0]->value.as_number->n.mpq);
  ArgonObject *output = new_number_object(result);
  mpq_clear(result);
  return output;
}

ArgonObject *ARGON_NUMBER_TYPE___add__(size_t argc, ArgonObject **argv,
                                       ArErr *err, RuntimeState *state,
                                       ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__add__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__add__ cannot perform addition between a number and %.*s",
        type_name->value.as_str->length, type_name->value.as_str->data);
    return ARGON_NULL;
  }
  if (argv[0]->value.as_number->is_int64 &&
      argv[1]->value.as_number->is_int64) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    bool gonna_overflow = (a > 0 && b > 0 && a > INT64_MAX - b) ||
                          (a < 0 && b < 0 && a < INT64_MIN - b);
    if (!gonna_overflow) {
      return new_number_object_from_int64(a + b);
    }
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    mpq_set_si(a_GMP, a, 1);
    mpq_set_si(b_GMP, b, 1);
    mpq_add(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_add(r, *argv[0]->value.as_number->n.mpq,
            *argv[1]->value.as_number->n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number->is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number->n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number->n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number->n.mpq);
      mpq_set_si(b_GMP, argv[1]->value.as_number->n.i64, 1);
    }
    mpq_add(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___subtract__(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state,
                                            ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__subtract__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__subtract__ cannot perform subtraction between number and %.*s",
        type_name->value.as_str->length, type_name->value.as_str->data);
    return ARGON_NULL;
  }

  if (argv[0]->value.as_number->is_int64 &&
      argv[1]->value.as_number->is_int64) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    int64_t neg_a = -a;
    bool gonna_overflow = (neg_a > 0 && b > 0 && b > INT64_MAX - neg_a) ||
                          (neg_a < 0 && b < 0 && b < INT64_MIN - neg_a);
    if (!gonna_overflow) {
      return new_number_object_from_int64(a - b);
    }
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    mpq_set_si(a_GMP, a, 1);
    mpq_set_si(b_GMP, b, 1);
    mpq_sub(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_sub(r, *argv[0]->value.as_number->n.mpq,
            *argv[1]->value.as_number->n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number->is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number->n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number->n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number->n.mpq);
      mpq_set_si(b_GMP, argv[1]->value.as_number->n.i64, 1);
    }
    mpq_sub(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___multiply__(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state,
                                            ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__multiply__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__multiply__ cannot perform multiplication between number and %.*s",
        type_name->value.as_str->length, type_name->value.as_str->data);
    return ARGON_NULL;
  }

  if (argv[0]->value.as_number->is_int64 &&
      argv[1]->value.as_number->is_int64) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    bool gonna_overflow =
        a > 0 ? (b > 0 ? a > INT64_MAX / b : b < INT64_MIN / a)
              : (b > 0 ? a < INT64_MIN / b : a != 0 && b < INT64_MAX / a);
    if (!gonna_overflow) {
      return new_number_object_from_int64(a * b);
    }
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    mpq_set_si(a_GMP, a, 1);
    mpq_set_si(b_GMP, b, 1);
    mpq_mul(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_mul(r, *argv[0]->value.as_number->n.mpq,
            *argv[1]->value.as_number->n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number->is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number->n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number->n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number->n.mpq);
      mpq_set_si(b_GMP, argv[1]->value.as_number->n.i64, 1);
    }
    mpq_mul(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___exponent__(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state,
                                            ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__exponent__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__exponent__ cannot perform multiplication between number and %.*s",
        type_name->value.as_str->length, type_name->value.as_str->data);
    return ARGON_NULL;
  }

  /* ---------- fast int64 ^ int64 path ---------- */
  if (argv[0]->value.as_number->is_int64 &&
      argv[1]->value.as_number->is_int64) {

    int64_t base = argv[0]->value.as_number->n.i64;
    int64_t exp = argv[1]->value.as_number->n.i64;

    /* negative exponent → rational */
    if (exp < 0) {
      mpq_t a, b, r;
      mpq_init(a);
      mpq_init(b);
      mpq_init(r);

      mpq_set_si(a, base, 1);
      mpq_set_si(b, exp, 1);

      mpq_pow_q(r, a, b);

      ArgonObject *result = new_number_object(r);

      mpq_clear(a);
      mpq_clear(b);
      mpq_clear(r);
      return result;
    }

    /* exp >= 0 → try int64 exponentiation */
    bool overflow = false;
    int64_t result = 1;
    int64_t a = base;
    int64_t e = exp;

    while (e > 0) {
      if (e & 1) {
        if (__builtin_mul_overflow(result, a, &result)) {
          overflow = true;
          break;
        }
      }
      e >>= 1;
      if (e && __builtin_mul_overflow(a, a, &a)) {
        overflow = true;
        break;
      }
    }

    if (!overflow) {
      return new_number_object_from_int64(result);
    }

    /* overflow → fall through to GMP */
  }

  /* ---------- GMP / rational path ---------- */
  {
    mpq_t a_GMP, b_GMP, r;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    mpq_init(r);

    /* load base */
    if (argv[0]->value.as_number->is_int64)
      mpq_set_si(a_GMP, argv[0]->value.as_number->n.i64, 1);
    else
      mpq_set(a_GMP, *argv[0]->value.as_number->n.mpq);

    /* load exponent */
    if (argv[1]->value.as_number->is_int64)
      mpq_set_si(b_GMP, argv[1]->value.as_number->n.i64, 1);
    else
      mpq_set(b_GMP, *argv[1]->value.as_number->n.mpq);

    /* ---------- REAL-NUMBER DOMAIN CHECK ---------- */

    /* 0^0 or 0^negative */
    if (mpq_sgn(a_GMP) == 0 && mpq_sgn(b_GMP) <= 0) {
      *err =
          create_err(state->source_location.line, state->source_location.column,
                     state->source_location.length, state->path, "Math Error",
                     "0 cannot be raised to zero or a negative power");

      mpq_clear(a_GMP);
      mpq_clear(b_GMP);
      mpq_clear(r);
      return ARGON_NULL;
    }

    /* negative base with non-integer exponent → complex */
    if (mpq_sgn(a_GMP) < 0 && mpz_cmp_ui(mpq_denref(b_GMP), 1) != 0) {
      *err = create_err(
          state->source_location.line, state->source_location.column,
          state->source_location.length, state->path, "Math Error",
          "Negative base with fractional exponent is not a real number");

      mpq_clear(a_GMP);
      mpq_clear(b_GMP);
      mpq_clear(r);
      return ARGON_NULL;
    }

    /* ---------- SAFE TO COMPUTE ---------- */

    mpq_pow_q(r, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(r);

    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    mpq_clear(r);
    return result;
  }
}

static inline uint64_t mix64(uint64_t x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return x;
}

uint64_t hash_mpz(const mpz_t z) {
  // Export to raw bytes (big-endian for consistency)
  size_t count;
  unsigned char *data = mpz_export(NULL, &count, 1, 1, 1, 0, z);

  // FNV-1a over bytes
  uint64_t h = 1469598103934665603ULL;
  for (size_t i = 0; i < count; i++) {
    h ^= data[i];
    h *= 1099511628211ULL;
  }

  // Include sign bit
  if (mpz_sgn(z) < 0)
    h = ~h;

  // Free the temporary buffer allocated by mpz_export
  free(data);

  return mix64(h);
}

uint64_t hash_mpq(mpq_t q) {
  uint64_t h_num = hash_mpz(mpq_numref(q));
  uint64_t h_den = hash_mpz(mpq_denref(q));

  // Combine using a standard 64-bit hash mix (boost-style)
  uint64_t h =
      h_num ^ (h_den + 0x9e3779b97f4a7c15ULL + (h_num << 6) + (h_num >> 2));
  return mix64(h);
}

ArgonObject *ARGON_NUMBER_TYPE___hash__(size_t argc, ArgonObject **argv,
                                        ArErr *err, RuntimeState *state,
                                        ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__hash__ expects 1 arguments, got %" PRIu64, argc);
  }
  uint64_t hash;
  if (argv[0]->value.as_number->is_int64) {
    hash = mix64(argv[0]->value.as_number->n.i64);
  } else {
    hash = hash_mpq(*argv[0]->value.as_number->n.mpq);
  }
  return new_number_object_from_int64(hash);
}

ArgonObject *ARGON_NUMBER_TYPE___division__(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state,
                                            ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__division__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__division__ cannot perform division between number and %.*s",
        type_name->value.as_str->length, type_name->value.as_str->data);
    return ARGON_NULL;
  }
  if (argv[0]->value.as_number->is_int64 &&
      argv[1]->value.as_number->is_int64) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    if (!b) {
      *err =
          create_err(state->source_location.line, state->source_location.column,
                     state->source_location.length, state->path,
                     "Zero Division Error", "division by zero");
      return NULL;
    }
    return new_number_object_from_num_and_den(a, b);
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_div(r, *argv[0]->value.as_number->n.mpq,
            *argv[1]->value.as_number->n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number->is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number->n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number->n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number->n.mpq);
      if (!argv[1]->value.as_number->n.i64) {
        *err = create_err(state->source_location.line,
                          state->source_location.column,
                          state->source_location.length, state->path,
                          "Zero Division Error", "division by zero");
        return NULL;
      }
      mpq_set_si(b_GMP, argv[1]->value.as_number->n.i64, 1);
    }
    mpq_div(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___floor_division__(size_t argc,
                                                  ArgonObject **argv,
                                                  ArErr *err,
                                                  RuntimeState *state,
                                                  ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__floor_division__ expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__floor_division__ cannot perform floor division "
                      "between number and %.*s",
                      type_name->value.as_str->length,
                      type_name->value.as_str->data);
    return ARGON_NULL;
  }
  if (argv[0]->value.as_number->is_int64 &&
      argv[1]->value.as_number->is_int64) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    if (!b) {
      *err =
          create_err(state->source_location.line, state->source_location.column,
                     state->source_location.length, state->path,
                     "Zero Division Error", "floor division by zero");
      return NULL;
    }
    return new_number_object_from_int64(a / b);
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_fdiv(r, *argv[0]->value.as_number->n.mpq,
             *argv[1]->value.as_number->n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number->is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number->n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number->n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number->n.mpq);
      if (!argv[1]->value.as_number->n.i64) {
        *err = create_err(state->source_location.line,
                          state->source_location.column,
                          state->source_location.length, state->path,
                          "Zero Division Error", "floor division by zero");
        return NULL;
      }
      mpq_set_si(b_GMP, argv[1]->value.as_number->n.i64, 1);
    }
    mpq_fdiv(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___modulo__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__modulo__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__modulo__ cannot perform modulo between number and %.*s",
        type_name->value.as_str->length, type_name->value.as_str->data);
    return ARGON_NULL;
  }
  if (argv[0]->value.as_number->is_int64 &&
      argv[1]->value.as_number->is_int64) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    if (!b) {
      *err =
          create_err(state->source_location.line, state->source_location.column,
                     state->source_location.length, state->path,
                     "Zero Division Error", "modulo by zero");
      return NULL;
    }
    return new_number_object_from_int64(a % b);
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_fmod(r, *argv[0]->value.as_number->n.mpq,
             *argv[1]->value.as_number->n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number->is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number->n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number->n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number->n.mpq);
      if (!argv[1]->value.as_number->n.i64) {
        *err = create_err(state->source_location.line,
                          state->source_location.column,
                          state->source_location.length, state->path,
                          "Zero Division Error", "modulo by zero");
        return NULL;
      }
      mpq_set_si(b_GMP, argv[1]->value.as_number->n.i64, 1);
    }
    mpq_fmod(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___equal__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state,
                                         ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    return ARGON_FALSE;
  }
  if (likely(argv[0]->value.as_number->is_int64 &&
             argv[1]->value.as_number->is_int64)) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    return a == b ? ARGON_TRUE : ARGON_FALSE;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {

    return mpq_cmp(*argv[0]->value.as_number->n.mpq,
                   *argv[1]->value.as_number->n.mpq) == 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else if (argv[0]->value.as_number->is_int64) {
    return mpq_cmp_ui(*argv[1]->value.as_number->n.mpq,
                      argv[0]->value.as_number->n.i64, 1) == 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else {
    return mpq_cmp_ui(*argv[0]->value.as_number->n.mpq,
                      argv[1]->value.as_number->n.i64, 1) == 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___not_equal__(size_t argc, ArgonObject **argv,
                                             ArErr *err, RuntimeState *state,
                                             ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__not_equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    return ARGON_TRUE;
  }
  if (likely(argv[0]->value.as_number->is_int64 &&
             argv[1]->value.as_number->is_int64)) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    return a != b ? ARGON_TRUE : ARGON_FALSE;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {

    return mpq_cmp(*argv[0]->value.as_number->n.mpq,
                   *argv[1]->value.as_number->n.mpq) != 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else if (argv[0]->value.as_number->is_int64) {
    return mpq_cmp_ui(*argv[1]->value.as_number->n.mpq,
                      argv[0]->value.as_number->n.i64, 1) != 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else {
    return mpq_cmp_ui(*argv[0]->value.as_number->n.mpq,
                      argv[1]->value.as_number->n.i64, 1) != 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___less_than__(size_t argc, ArgonObject **argv,
                                             ArErr *err, RuntimeState *state,
                                             ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__less_than__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "cannot perform < between number and %.*s",
                      type_name->value.as_str->length,
                      type_name->value.as_str->data);
    return ARGON_NULL;
  }
  if (likely(argv[0]->value.as_number->is_int64 &&
             argv[1]->value.as_number->is_int64)) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    return a < b ? ARGON_TRUE : ARGON_FALSE;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {

    return mpq_cmp(*argv[0]->value.as_number->n.mpq,
                   *argv[1]->value.as_number->n.mpq) < 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else if (argv[0]->value.as_number->is_int64) {
    return mpq_cmp_ui(*argv[1]->value.as_number->n.mpq,
                      argv[0]->value.as_number->n.i64, 1) > 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else {
    return mpq_cmp_ui(*argv[0]->value.as_number->n.mpq,
                      argv[1]->value.as_number->n.i64, 1) < 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___less_than_equal__(size_t argc,
                                                   ArgonObject **argv,
                                                   ArErr *err,
                                                   RuntimeState *state,
                                                   ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__less_than_equal__ expects 2 arguments, got %" PRIu64,
                      argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "cannot perform <= between number and %.*s",
                      type_name->value.as_str->length,
                      type_name->value.as_str->data);
    return ARGON_NULL;
  }
  if (likely(argv[0]->value.as_number->is_int64 &&
             argv[1]->value.as_number->is_int64)) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    return a <= b ? ARGON_TRUE : ARGON_FALSE;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {

    return mpq_cmp(*argv[0]->value.as_number->n.mpq,
                   *argv[1]->value.as_number->n.mpq) <= 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else if (argv[0]->value.as_number->is_int64) {
    return mpq_cmp_ui(*argv[1]->value.as_number->n.mpq,
                      argv[0]->value.as_number->n.i64, 1) >= 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else {
    return mpq_cmp_ui(*argv[0]->value.as_number->n.mpq,
                      argv[1]->value.as_number->n.i64, 1) <= 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___greater_than__(size_t argc, ArgonObject **argv,
                                                ArErr *err, RuntimeState *state,
                                                ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err =
        create_err(0, 0, 0, "", "Runtime Error",
                   "__greater_than__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "cannot perform > between number and %.*s",
                      type_name->value.as_str->length,
                      type_name->value.as_str->data);
    return ARGON_NULL;
  }
  if (likely(argv[0]->value.as_number->is_int64 &&
             argv[1]->value.as_number->is_int64)) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    return a > b ? ARGON_TRUE : ARGON_FALSE;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {

    return mpq_cmp(*argv[0]->value.as_number->n.mpq,
                   *argv[1]->value.as_number->n.mpq) > 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else if (argv[0]->value.as_number->is_int64) {
    return mpq_cmp_ui(*argv[1]->value.as_number->n.mpq,
                      argv[0]->value.as_number->n.i64, 1) < 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else {
    return mpq_cmp_ui(*argv[0]->value.as_number->n.mpq,
                      argv[1]->value.as_number->n.i64, 1) > 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___greater_than_equal__(size_t argc,
                                                      ArgonObject **argv,
                                                      ArErr *err,
                                                      RuntimeState *state,
                                                      ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 2) {
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__greater_than_equal__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__), __name__, argv[1]);
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "cannot perform >= between number and %.*s",
                      type_name->value.as_str->length,
                      type_name->value.as_str->data);
    return ARGON_NULL;
  }
  if (likely(argv[0]->value.as_number->is_int64 &&
             argv[1]->value.as_number->is_int64)) {
    int64_t a = argv[0]->value.as_number->n.i64;
    int64_t b = argv[1]->value.as_number->n.i64;
    return a >= b ? ARGON_TRUE : ARGON_FALSE;
  } else if (!argv[0]->value.as_number->is_int64 &&
             !argv[1]->value.as_number->is_int64) {

    return mpq_cmp(*argv[0]->value.as_number->n.mpq,
                   *argv[1]->value.as_number->n.mpq) >= 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else if (argv[0]->value.as_number->is_int64) {
    return mpq_cmp_ui(*argv[1]->value.as_number->n.mpq,
                      argv[0]->value.as_number->n.i64, 1) <= 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  } else {
    return mpq_cmp_ui(*argv[0]->value.as_number->n.mpq,
                      argv[1]->value.as_number->n.i64, 1) >= 0
               ? ARGON_TRUE
               : ARGON_FALSE;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___string__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api) {
  (void)api;
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 arguments, got %" PRIu64, argc);
    return NULL;
  }

  if (argv[0]->value.as_number->is_int64) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%" PRId64, argv[0]->value.as_number->n.i64);
    return new_string_object_null_terminated(buf);
  }

  mpq_t *num = argv[0]->value.as_number->n.mpq;

  /* If denominator == 1, print numerator as full integer */
  if (mpz_cmp_ui(mpq_denref(*num), 1) == 0) {
    char *num_str =
        mpz_get_str(NULL, 10, mpq_numref(*num)); /* malloc'd by GMP */
    ArgonObject *result = new_string_object_null_terminated(num_str);
    free(num_str);
    return result;
  }

  /* Not an integer: use mpf to format with SIGNIFICANT_DIGITS precision */
  mpf_t f;
  mpf_init(f);
  mpf_set_q(f, *num); /* set mpf from mpq */

  mp_exp_t exp; /* exponent returned by mpf_get_str */
  /* Request SIGNIFICANT_DIGITS significant digits. If you want "max accurate",
   * pass 0. */
  char *mant = mpf_get_str(NULL, &exp, 10, SIGNIFICANT_DIGITS, f);
  /* For zero, mpf_get_str returns an empty string and exp == 0 per GMP docs. */
  if (mant == NULL) {
    mpf_clear(f);
    return new_string_object_null_terminated("0");
  }

  /* handle zero specially */
  if (mant[0] == '\0' || (mant[0] == '0' && mant[1] == '\0')) {
    free(mant);
    mpf_clear(f);
    return new_string_object_null_terminated("0");
  }

  /* mant may include a leading '-' according to some docs; detect sign */
  int negative = 0;
  char *digits = mant;
  if (mant[0] == '-') {
    negative = 1;
    digits = mant + 1;
  }

  size_t L = strlen(digits); /* number of digit characters returned */
  /* mpf_get_str represents value as 0.digits * 10^exp (i.e. assumed decimal
   * point after the leading zero) */
  /* For scientific-format exponent (1.d..eE) we use scientific_exponent = exp -
   * 1 */
  long scientific_exp = (long)exp - 1L;

  /* Decide whether to use fixed or scientific, mimic C's %g rule:
     use scientific if exponent < -4 or exponent >= SIGNIFICANT_DIGITS */
  int use_scientific =
      (scientific_exp < -4) || (scientific_exp >= SIGNIFICANT_DIGITS);

  /* Build output into dynamic buffer */
  /* Worst-case: sign + 1 digit + '.' + (SIGNIFICANT_DIGITS-1) digits + 'e' +
   * sign + exponent digits + NUL */
  size_t buf_size = (size_t)(negative ? 1 : 0) + 1 + 1 +
                    (SIGNIFICANT_DIGITS - 1) + 1 + 1 + 32 + 1;
  /* For fixed form we may need more if exp > L (we append zeros). Allocate a
   * bit extra. */
  if (!use_scientific) {
    /* maximum integer digits = max(exp, L) but exp could be large; be
     * conservative */
    buf_size += (size_t)((exp > (mp_exp_t)L) ? (size_t)exp : L) + 16;
  }
  char *out = malloc(buf_size);
  if (!out) {
    free(mant);
    mpf_clear(f);
    *err = create_err(0, 0, 0, "", "Runtime Error", "out of memory");
    return NULL;
  }

  char *p = out;
  if (negative) {
    *p++ = '-';
  }

  if (use_scientific) {
    /* scientific:  d.dddddeE  where d = digits[0], fractional = digits[1..L-1]
     */
    *p++ = digits[0];
    if (L > 1) {
      *p++ = '.';
      memcpy(p, digits + 1, L - 1);
      p += L - 1;
    }
    /* append exponent */
    int written = snprintf(p, buf_size - (p - out), "e%+ld", scientific_exp);
    if (written < 0)
      written = 0;
    p += written;
  } else {
    /* fixed form: move decimal point right by 'exp' places in 0.digits * 10^exp
     */
    /* integer part length = exp (may be <=0 meaning 0) */
    long int_len = (long)exp;
    if (int_len <= 0) {
      /* 0.xxx... form */
      *p++ = '0';
      *p++ = '.';
      /* need (-int_len) leading zeros after decimal */
      for (long i = 0; i < -int_len; ++i)
        *p++ = '0';
      /* then digits */
      memcpy(p, digits, L);
      p += L;
    } else {
      /* integer part uses first int_len digits of digits (if available), else
       * digits plus zeros */
      if ((size_t)int_len <= L) {
        /* put first int_len digits as integer part */
        memcpy(p, digits, (size_t)int_len);
        p += int_len;
        /* fractional part exists if L > int_len */
        if (L > (size_t)int_len) {
          *p++ = '.';
          memcpy(p, digits + int_len, L - int_len);
          p += L - int_len;
        }
      } else {
        /* digits provide only part of integer, append zeros */
        memcpy(p, digits, L);
        p += L;
        for (long i = 0; i < int_len - (long)L; ++i)
          *p++ = '0';
        /* no fractional part */
      }
    }
  }

  *p = '\0';

  /* Clean up */
  free(mant);
  mpf_clear(f);

  ArgonObject *result = new_string_object_null_terminated(out);
  free(out);
  return result;
}

ArgonObject small_ints[small_ints_max - small_ints_min + 1];
struct as_number small_ints_as_number[small_ints_max - small_ints_min + 1];

void init_small_ints() {
  for (int64_t i = 0; i <= small_ints_max - small_ints_min; i++) {
    int64_t n = i + small_ints_min;
    small_ints[i].type = TYPE_NUMBER;
    small_ints[i].dict = NULL;
    small_ints[i].value.as_number = &small_ints_as_number[i];
    add_builtin_field(&small_ints[i], __class__, ARGON_NUMBER_TYPE);
    small_ints[i].value.as_number->is_int64 = true;
    small_ints[i].value.as_number->n.i64 = n;
    small_ints[i].as_bool = n;
  }
}

void create_ARGON_NUMBER_TYPE() {
  ARGON_NUMBER_TYPE = new_class();
  add_builtin_field(ARGON_NUMBER_TYPE, __name__,
                    new_string_object_null_terminated("number"));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __string__,
      create_argon_native_function("__string__", ARGON_NUMBER_TYPE___string__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __new__,
      create_argon_native_function("__new__", ARGON_NUMBER_TYPE___new__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __number__,
      create_argon_native_function("__number__", ARGON_NUMBER_TYPE___number__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __hash__,
      create_argon_native_function("__hash__", ARGON_NUMBER_TYPE___hash__));
  add_builtin_field(ARGON_NUMBER_TYPE, __boolean__,
                    create_argon_native_function(
                        "__boolean__", ARGON_NUMBER_TYPE___boolean__));
  add_builtin_field(ARGON_NUMBER_TYPE, __negation__,
                    create_argon_native_function(
                        "__negation__", ARGON_NUMBER_TYPE___negation__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __add__,
      create_argon_native_function("__add__", ARGON_NUMBER_TYPE___add__));
  add_builtin_field(ARGON_NUMBER_TYPE, __subtract__,
                    create_argon_native_function(
                        "__subtract__", ARGON_NUMBER_TYPE___subtract__));
  add_builtin_field(ARGON_NUMBER_TYPE, __multiply__,
                    create_argon_native_function(
                        "__multiply__", ARGON_NUMBER_TYPE___multiply__));
  add_builtin_field(ARGON_NUMBER_TYPE, __exponent__,
                    create_argon_native_function(
                        "__exponent__", ARGON_NUMBER_TYPE___exponent__));
  add_builtin_field(ARGON_NUMBER_TYPE, __division__,
                    create_argon_native_function(
                        "__division__", ARGON_NUMBER_TYPE___division__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __floor_division__,
      create_argon_native_function("__floor_division__",
                                   ARGON_NUMBER_TYPE___floor_division__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __modulo__,
      create_argon_native_function("__modulo__", ARGON_NUMBER_TYPE___modulo__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __equal__,
      create_argon_native_function("__equal__", ARGON_NUMBER_TYPE___equal__));
  add_builtin_field(ARGON_NUMBER_TYPE, __not_equal__,
                    create_argon_native_function(
                        "__not_equal__", ARGON_NUMBER_TYPE___not_equal__));
  add_builtin_field(ARGON_NUMBER_TYPE, __less_than__,
                    create_argon_native_function(
                        "__less_than__", ARGON_NUMBER_TYPE___less_than__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __less_than_equal__,
      create_argon_native_function("__less_than_equal__",
                                   ARGON_NUMBER_TYPE___less_than_equal__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __greater_than__,
      create_argon_native_function("__greater_than__",
                                   ARGON_NUMBER_TYPE___greater_than__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __greater_than_equal__,
      create_argon_native_function("__greater_than_equal__",
                                   ARGON_NUMBER_TYPE___greater_than_equal__));
  init_small_ints();
}

void mpz_init_gc_managed(mpz_t z, size_t limbs_count) {
  z->_mp_alloc = limbs_count;
  z->_mp_size = 0;
  z->_mp_d = ar_alloc_atomic(limbs_count * sizeof(mp_limb_t));
}

void mpq_init_gc_managed(mpq_t q, size_t num_limbs, size_t den_limbs) {
  mpz_init_gc_managed(mpq_numref(q), num_limbs);
  mpz_init_gc_managed(mpq_denref(q), den_limbs);
  mpq_set_ui(q, 0, 1); // initialize denominator to 1
}

void mpq_copy_to_gc(mpq_t dest, const mpq_t src) {
  size_t num_limbs = (size_t)abs(mpq_numref(src)->_mp_size);
  size_t den_limbs = (size_t)abs(mpq_denref(src)->_mp_size);

  dest->_mp_num._mp_size = mpq_numref(src)->_mp_size;
  memcpy(dest->_mp_num._mp_d, mpq_numref(src)->_mp_d,
         num_limbs * sizeof(mp_limb_t));

  dest->_mp_den._mp_size = mpq_denref(src)->_mp_size;
  memcpy(dest->_mp_den._mp_d, mpq_denref(src)->_mp_d,
         den_limbs * sizeof(mp_limb_t));
}

mpq_t *mpq_new_gc_from(const mpq_t src) {
  mpq_t *dest = ar_alloc(sizeof(mpq_t));

  size_t num_limbs = (size_t)mpq_numref(src)->_mp_alloc;
  size_t den_limbs = (size_t)mpq_denref(src)->_mp_alloc;

  mpq_init_gc_managed(*dest, num_limbs, den_limbs);
  mpq_copy_to_gc(*dest, src);

  return dest;
}

bool mpq_to_int64(mpq_t q, int64_t *out) {
  // Check denominator == 1
  if (mpz_cmp_ui(mpq_denref(q), 1) != 0) {
    return false;
  }

  // Get numerator
  mpz_t num;
  mpz_init(num);
  mpz_set(num, mpq_numref(q));

  // Check bounds
  if (mpz_cmp_si(num, INT64_MIN) < 0 || mpz_cmp_si(num, INT64_MAX) > 0) {
    mpz_clear(num);
    return false;
  }

  *out = mpz_get_si(num); // safe because we checked range
  mpz_clear(num);
  return true;
}

bool double_to_int64(double x, int64_t *out) {
  if (x < (double)INT64_MIN || x > (double)INT64_MAX) {
    return false;
  }

  int64_t i = (int64_t)x;
  if ((double)i == x) { // no fractional part
    *out = i;
    return true;
  }
  return false;
}

static void mpq_pow_z(mpq_t rop, const mpq_t base, const mpz_t exp) {
  mpq_t result, tmp;
  mpz_t e;

  mpq_init(result);
  mpq_init(tmp);
  mpz_init_set(e, exp);

  mpq_set_ui(result, 1, 1); // result = 1
  mpq_set(tmp, base);

  int negative = mpz_sgn(e) < 0;
  if (negative) {
    mpz_neg(e, e);
  }

  while (mpz_cmp_ui(e, 0) > 0) {
    if (mpz_odd_p(e)) {
      mpq_mul(result, result, tmp);
    }
    mpq_mul(tmp, tmp, tmp);
    mpz_fdiv_q_2exp(e, e, 1);
  }

  if (negative) {
    mpq_inv(result, result);
  }

  mpq_set(rop, result);

  mpq_clear(result);
  mpq_clear(tmp);
  mpz_clear(e);
}

void mpq_pow_q(mpq_t rop, const mpq_t e, const mpq_t n) {
  // Case 1: integer exponent → exact
  if (mpz_cmp_ui(mpq_denref(n), 1) == 0) {
    mpq_pow_z(rop, e, mpq_numref(n));
    return;
  }

  // n = p / q
  mpz_t p, q;
  mpz_init_set(p, mpq_numref(n));
  mpz_init_set(q, mpq_denref(n));

  // Precision policy
  mpfr_prec_t prec = 256;
  prec += mpz_sizeinbase(mpq_numref(e), 2);
  prec += mpz_sizeinbase(mpq_denref(e), 2);
  prec += mpz_sizeinbase(p, 2);

  mpfr_t fe, root;
  mpfr_init2(fe, prec);
  mpfr_init2(root, prec);

  // fe = e
  mpfr_set_q(fe, e, MPFR_RNDN);

  // root = e^(1/q)
  unsigned long q_ui = mpz_get_ui(q);
  mpfr_rootn_ui(root, fe, q_ui, MPFR_RNDN);

  // root = root^p
  mpfr_pow_z(root, root, p, MPFR_RNDN);

  // Convert to rational approximation
  mpfr_get_q(rop, root);

  mpfr_clear(fe);
  mpfr_clear(root);
  mpz_clear(p);
  mpz_clear(q);
}

ArgonObject *new_number_object(mpq_t number) {
  int64_t i64 = 0;
  bool is_int64 = mpq_to_int64(number, &i64);
  if (is_int64 && i64 >= small_ints_min && i64 <= small_ints_max) {
    return &small_ints[i64 - small_ints_min];
  }
  ArgonObject *object;
  if (is_int64) {
    object = new_small_instance(ARGON_NUMBER_TYPE,sizeof(struct as_number));
  } else {
    object = new_instance(ARGON_NUMBER_TYPE,sizeof(struct as_number));
  }
  object->value.as_number = (struct as_number*)((char*)object+sizeof(ArgonObject));
  object->type = TYPE_NUMBER;
  object->value.as_number->n.i64 = i64;
  object->value.as_number->is_int64 = is_int64;
  if (object->value.as_number->is_int64) {
    object->as_bool = object->value.as_number->n.i64;
  } else {
    object->value.as_number->n.mpq = mpq_new_gc_from(number);
    object->as_bool = mpq_cmp_si(number, 0, 1) != 0;
  }
  return object;
}

ArgonObject *new_number_object_from_num_and_den(int64_t n, uint64_t d) {
  if (d == 1 && n >= small_ints_min && n <= small_ints_max) {
    return &small_ints[n - small_ints_min];
  }
  ArgonObject *object = new_small_instance(ARGON_NUMBER_TYPE, sizeof(struct as_number));
  object->value.as_number = (struct as_number*)((char*)object+sizeof(ArgonObject));
  object->type = TYPE_NUMBER;
  if (d == 1) {
    object->value.as_number->is_int64 = true;
    object->value.as_number->n.i64 = n;
    object->as_bool = n;
  } else {
    object->value.as_number->is_int64 = false;
    mpq_t r;
    mpq_init(r);
    mpq_set_si(r, n, d);
    object->value.as_number->n.mpq = mpq_new_gc_from(r);
    object->as_bool = n != 0;
    mpq_clear(r);
  }
  return object;
}

ArgonObject *new_number_object_from_int64(int64_t i64) {
  if (i64 >= small_ints_min && i64 <= small_ints_max) {
    return &small_ints[i64 - small_ints_min];
  }
  ArgonObject *object = new_small_instance(ARGON_NUMBER_TYPE, sizeof(struct as_number));
  object->value.as_number = (struct as_number*)((char*)object+sizeof(ArgonObject));
  object->type = TYPE_NUMBER;
  object->value.as_number->is_int64 = true;
  object->value.as_number->n.i64 = i64;
  object->as_bool = i64;
  return object;
}

ArgonObject *new_number_object_from_double(double d) {
  int64_t i64 = 0;
  bool is_int64 = double_to_int64(d, &i64);
  if (is_int64 && i64 >= small_ints_min && i64 <= small_ints_max) {
    return &small_ints[i64 - small_ints_min];
  }
  ArgonObject *object = new_small_instance(ARGON_NUMBER_TYPE, sizeof(struct as_number));
  object->value.as_number = (struct as_number*)((char*)object+sizeof(ArgonObject));
  object->type = TYPE_NUMBER;
  object->value.as_number->n.i64 = i64;
  object->value.as_number->is_int64 = is_int64;
  if (object->value.as_number->is_int64) {
    object->as_bool = object->value.as_number->n.i64;
  } else {
    mpq_t r;
    mpq_init(r);
    mpq_set_d(r, d);
    object->value.as_number->n.mpq = mpq_new_gc_from(r);
    object->as_bool = d != 0;
    mpq_clear(r);
  }
  return object;
}

static inline uint64_t hash_u64(uint64_t h, uint64_t x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return h ^ (x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
}

uint64_t make_id(size_t num_size, size_t num_pos, bool is_int, size_t den_size,
                 size_t den_pos) {
  uint64_t h = 0;
  h = hash_u64(h, num_size);
  h = hash_u64(h, num_pos);
  h = hash_u64(h, is_int);
  if (!is_int) {
    h = hash_u64(h, den_size);
    h = hash_u64(h, den_pos);
  }
  return h;
}