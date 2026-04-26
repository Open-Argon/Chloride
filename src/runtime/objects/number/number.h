/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_NUMBER_H
#define RUNTIME_NUMBER_H
#include "../object.h"

#define small_ints_min -256
#define small_ints_max 256

extern ArgonObject *ARGON_NUMBER_TYPE;

void mpq_set_si64(mpq_t q, int64_t n, int64_t d);

void create_ARGON_NUMBER_TYPE();

ArgonObject *new_number_object(mpq_t number);

bool mpq_to_int64(mpq_t q, int64_t *out);

ArgonObject *new_number_object_from_double(double d);

ArgonObject *new_number_object_from_num_and_den(int64_t n, int64_t d);

ArgonObject *new_number_object_from_int64(int64_t i64);

void mpq_pow_q(mpq_t rop, const mpq_t e, const mpq_t n);

void mpq_fdiv(mpq_t result, const mpq_t a, const mpq_t b);

void mpq_fmod(mpq_t result, const mpq_t a, const mpq_t b);

uint64_t make_id(size_t num_size, size_t num_pos, bool is_int, bool is_negative,
                 size_t den_size, size_t den_pos);

EXPOSE_ARGON_METHOD(ARGON_NUMBER_TYPE, __equal__)

EXPOSE_ARGON_METHOD(ARGON_NUMBER_TYPE, __add__)

EXPOSE_ARGON_METHOD(ARGON_NUMBER_TYPE, __greater_than_equal__)

EXPOSE_ARGON_METHOD(ARGON_NUMBER_TYPE, __greater_than__)

EXPOSE_ARGON_METHOD(ARGON_NUMBER_TYPE, __less_than_equal__)

EXPOSE_ARGON_METHOD(ARGON_NUMBER_TYPE, __less_than__)

struct small_ints_struct {
  struct as_number as_number;
  ArgonObject obj;
};
extern struct small_ints_struct small_ints[small_ints_max - small_ints_min + 1];

#endif // RUNTIME_NUMBER_H