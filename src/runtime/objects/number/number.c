/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "number.h"
#include "../functions/functions.h"
#include "../string/string.h"
#include <gmp.h>
#include <inttypes.h>
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

/* change SIGNIFICANT_DIGITS to taste (15 mimics double-ish behaviour) */
#define SIGNIFICANT_DIGITS 15

ArgonObject *ARGON_NUMBER_TYPE___new__(size_t argc, ArgonObject **argv,
                                       ArErr *err, RuntimeState *state) {
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__new__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *object = argv[1];

  self->type = TYPE_STRING;
  ArgonObject *boolean_convert_method = get_builtin_field_for_class(
      get_builtin_field(object, __class__, false, false), __number__, object);
  if (boolean_convert_method) {
    ArgonObject *boolean_object =
        argon_call(boolean_convert_method, 0, NULL, err, state);
    if (err->exists)
      return ARGON_NULL;
    return boolean_object;
  }
  ArgonObject *type_name = get_builtin_field_for_class(
      get_builtin_field(object, __class__, false, false), __name__, object);
  *err = create_err(
      0, 0, 0, "", "Runtime Error", "cannot convert type '%.*s' to number",
      type_name->value.as_str.length, type_name->value.as_str.data);
  return ARGON_NULL;
}

ArgonObject *ARGON_NUMBER_TYPE___number__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__number__ expects 1 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return argv[0];
}

ArgonObject *ARGON_NUMBER_TYPE___boolean__(size_t argc, ArgonObject **argv,
                                           ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__boolean__ expects 1 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  return argv[0]->as_bool ? ARGON_FALSE : ARGON_TRUE;
}

ArgonObject *ARGON_NUMBER_TYPE___add__(size_t argc, ArgonObject **argv,
                                       ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__add__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__, false, false), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__add__ cannot perform addition between a number and %.*s",
        type_name->value.as_str.length, type_name->value.as_str.data);
    return ARGON_NULL;
  }
  if (argv[0]->value.as_number.is_int64 && argv[1]->value.as_number.is_int64) {
    int64_t a = argv[0]->value.as_number.n.i64;
    int64_t b = argv[1]->value.as_number.n.i64;
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
  } else if (!argv[0]->value.as_number.is_int64 &&
             !argv[1]->value.as_number.is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_add(r, *argv[0]->value.as_number.n.mpq,
            *argv[1]->value.as_number.n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number.is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number.n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number.n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number.n.mpq);
      mpq_set_si(b_GMP, argv[1]->value.as_number.n.i64, 1);
    }
    mpq_add(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___subtract__(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__subtract__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__, false, false), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__subtract__ cannot perform subtraction between number and %.*s",
        type_name->value.as_str.length, type_name->value.as_str.data);
    return ARGON_NULL;
  }

  if (argv[0]->value.as_number.is_int64 && argv[1]->value.as_number.is_int64) {
    int64_t a = argv[0]->value.as_number.n.i64;
    int64_t b = argv[1]->value.as_number.n.i64;
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
  } else if (!argv[0]->value.as_number.is_int64 &&
             !argv[1]->value.as_number.is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_sub(r, *argv[0]->value.as_number.n.mpq,
            *argv[1]->value.as_number.n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number.is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number.n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number.n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number.n.mpq);
      mpq_set_si(b_GMP, argv[1]->value.as_number.n.i64, 1);
    }
    mpq_sub(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___multiply__(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__multiply__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__, false, false), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__multiply__ cannot perform multiplication between number and %.*s",
        type_name->value.as_str.length, type_name->value.as_str.data);
    return ARGON_NULL;
  }

  if (argv[0]->value.as_number.is_int64 && argv[1]->value.as_number.is_int64) {
    int64_t a = argv[0]->value.as_number.n.i64;
    int64_t b = argv[1]->value.as_number.n.i64;
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
  } else if (!argv[0]->value.as_number.is_int64 &&
             !argv[1]->value.as_number.is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_mul(r, *argv[0]->value.as_number.n.mpq,
            *argv[1]->value.as_number.n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number.is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number.n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number.n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number.n.mpq);
      mpq_set_si(b_GMP, argv[1]->value.as_number.n.i64, 1);
    }
    mpq_mul(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___division__(size_t argc, ArgonObject **argv,
                                            ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 2) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__division__ expects 2 arguments, got %" PRIu64, argc);
    return ARGON_NULL;
  }
  if (argv[1]->type != TYPE_NUMBER) {
    ArgonObject *type_name = get_builtin_field_for_class(
        get_builtin_field(argv[1], __class__, false, false), __name__, argv[1]);
    *err = create_err(
        0, 0, 0, "", "Runtime Error",
        "__division__ cannot perform division between number and %.*s",
        type_name->value.as_str.length, type_name->value.as_str.data);
    return ARGON_NULL;
  }
  if (argv[0]->value.as_number.is_int64 && argv[1]->value.as_number.is_int64) {
    int64_t a = argv[0]->value.as_number.n.i64;
    int64_t b = argv[1]->value.as_number.n.i64;
    return new_number_object_from_num_and_den(a, b);
  } else if (!argv[0]->value.as_number.is_int64 &&
             !argv[1]->value.as_number.is_int64) {
    mpq_t r;
    mpq_init(r);
    mpq_div(r, *argv[0]->value.as_number.n.mpq,
            *argv[1]->value.as_number.n.mpq);
    ArgonObject *result = new_number_object(r);
    mpq_clear(r);
    return result;
  } else {
    mpq_t a_GMP, b_GMP;
    mpq_init(a_GMP);
    mpq_init(b_GMP);
    if (argv[0]->value.as_number.is_int64) {
      mpq_set_si(a_GMP, argv[0]->value.as_number.n.i64, 1);
      mpq_set(b_GMP, *argv[1]->value.as_number.n.mpq);
    } else {
      mpq_set(a_GMP, *argv[0]->value.as_number.n.mpq);
      mpq_set_si(b_GMP, argv[1]->value.as_number.n.i64, 1);
    }
    mpq_div(a_GMP, a_GMP, b_GMP);
    ArgonObject *result = new_number_object(a_GMP);
    mpq_clear(a_GMP);
    mpq_clear(b_GMP);
    return result;
  }
}

ArgonObject *ARGON_NUMBER_TYPE___string__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state) {
  (void)state;
  if (argc != 1) {
    *err = create_err(0, 0, 0, "", "Runtime Error",
                      "__string__ expects 1 arguments, got %" PRIu64, argc);
    return NULL;
  }

  if (argv[0]->value.as_number.is_int64) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%" PRId64, argv[0]->value.as_number.n.i64);
    return new_string_object_null_terminated(buf);
  }

  mpq_t *num = argv[0]->value.as_number.n.mpq;

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

#define small_ints_min -256
#define small_ints_max 256
ArgonObject small_ints[small_ints_max - small_ints_min + 1];

void init_small_ints() {
  for (int64_t i = 0; i <= small_ints_max - small_ints_min; i++) {
    int64_t n = i + small_ints_min;
    small_ints[i].type = TYPE_NUMBER;
    small_ints[i].dict = createHashmap_GC();
    add_builtin_field(&small_ints[i], __class__, ARGON_NUMBER_TYPE);
    add_builtin_field(&small_ints[i], __base__, BASE_CLASS);
    small_ints[i].value.as_number.is_int64 = true;
    small_ints[i].value.as_number.n.i64 = n;
    small_ints[i].as_bool = n;
  }
}

void create_ARGON_NUMBER_TYPE() {
  ARGON_NUMBER_TYPE = new_object();
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
  add_builtin_field(ARGON_NUMBER_TYPE, __boolean__,
                    create_argon_native_function(
                        "__boolean__", ARGON_NUMBER_TYPE___boolean__));
  add_builtin_field(
      ARGON_NUMBER_TYPE, __add__,
      create_argon_native_function("__add__", ARGON_NUMBER_TYPE___add__));
  add_builtin_field(ARGON_NUMBER_TYPE, __subtract__,
                    create_argon_native_function(
                        "__subtract__", ARGON_NUMBER_TYPE___subtract__));
  add_builtin_field(ARGON_NUMBER_TYPE, __multiply__,
                    create_argon_native_function(
                        "__multiply__", ARGON_NUMBER_TYPE___multiply__));
  add_builtin_field(ARGON_NUMBER_TYPE, __division__,
                    create_argon_native_function(
                        "__division__", ARGON_NUMBER_TYPE___division__));
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

ArgonObject *new_number_object(mpq_t number) {
  int64_t i64 = 0;
  bool is_int64 = mpq_to_int64(number, &i64);
  if (is_int64 && i64 >= small_ints_min && i64 <= small_ints_max) {
    return &small_ints[i64 - small_ints_min];
  }
  ArgonObject *object = new_object();
  add_builtin_field(object, __class__, ARGON_NUMBER_TYPE);
  object->type = TYPE_NUMBER;
  object->value.as_number.n.i64 = i64;
  object->value.as_number.is_int64 = is_int64;
  if (object->value.as_number.is_int64) {
    object->as_bool = object->value.as_number.n.i64;
  } else {
    object->value.as_number.n.mpq = mpq_new_gc_from(number);
    object->as_bool = mpq_cmp_si(number, 0, 1) != 0;
  }
  return object;
}

ArgonObject *new_number_object_from_num_and_den(int64_t n, uint64_t d) {
  if (d == 1 && n >= small_ints_min && n <= small_ints_max) {
    return &small_ints[n - small_ints_min];
  }
  ArgonObject *object = new_object();
  add_builtin_field(object, __class__, ARGON_NUMBER_TYPE);
  object->type = TYPE_NUMBER;
  if (d == 1) {
    object->value.as_number.is_int64 = true;
    object->value.as_number.n.i64 = n;
    object->as_bool = n;
  } else {
    object->value.as_number.is_int64 = false;
    mpq_t r;
    mpq_init(r);
    mpq_set_si(r, n, d);
    object->value.as_number.n.mpq = mpq_new_gc_from(r);
    object->as_bool = n != 0;
    mpq_clear(r);
  }
  return object;
}

ArgonObject *new_number_object_from_int64(int64_t i64) {
  if (i64 >= small_ints_min && i64 <= small_ints_max) {
    return &small_ints[i64 - small_ints_min];
  }
  ArgonObject *object = new_object();
  add_builtin_field(object, __class__, ARGON_NUMBER_TYPE);
  object->type = TYPE_NUMBER;
  object->value.as_number.is_int64 = true;
  object->value.as_number.n.i64 = i64;
  object->as_bool = i64;
  return object;
}

ArgonObject *new_number_object_from_double(double d) {
  int64_t i64 = 0;
  bool is_int64 = double_to_int64(d, &i64);
  if (is_int64 && i64 >= small_ints_min && i64 <= small_ints_max) {
    return &small_ints[i64 - small_ints_min];
  }
  ArgonObject *object = new_object();
  add_builtin_field(object, __class__, ARGON_NUMBER_TYPE);
  object->type = TYPE_NUMBER;
  object->value.as_number.n.i64 = i64;
  object->value.as_number.is_int64 = is_int64;
  if (object->value.as_number.is_int64) {
    object->as_bool = object->value.as_number.n.i64;
  } else {
    mpq_t r;
    mpq_init(r);
    mpq_set_d(r, d);
    object->value.as_number.n.mpq = mpq_new_gc_from(r);
    object->as_bool = d != 0;
    mpq_clear(r);
  }
  return object;
}

void load_number(Translated *translated, RuntimeState *state) {
  uint8_t to_register = pop_byte(translated, state);
  uint8_t is_int64 = pop_byte(translated, state);
  if (is_int64) {
    state->registers[to_register] =
        new_number_object_from_int64(pop_bytecode(translated, state));
    return;
  }
  size_t num_size = pop_bytecode(translated, state);
  size_t num_pos = pop_bytecode(translated, state);
  mpq_t r;
  mpq_init(r);
  mpz_t num;
  mpz_init(num);
  mpz_import(num, num_size, 1, 1, 0, 0,
             arena_get(&translated->constants, num_pos));
  mpq_set_num(r, num);
  mpz_clear(num);

  bool is_int = pop_byte(translated, state);

  if (!is_int) {
    size_t den_size = pop_bytecode(translated, state);
    size_t den_pos = pop_bytecode(translated, state);
    mpz_t den;
    mpz_init(den);
    mpz_import(den, den_size, 1, 1, 0, 0,
               arena_get(&translated->constants, den_pos));
    mpq_set_den(r, den);
    mpz_clear(den);
  } else {
    mpz_set_si(mpq_denref(r), 1);
  }

  state->registers[to_register] = new_number_object(r);
  mpq_clear(r);
}