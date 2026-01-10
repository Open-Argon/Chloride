/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_NUMBER_H
#define RUNTIME_NUMBER_H
#include "../object.h"

extern ArgonObject *ARGON_NUMBER_TYPE;

void create_ARGON_NUMBER_TYPE();

ArgonObject *new_number_object(mpq_t number);

bool mpq_to_int64(mpq_t q, int64_t *out);

void load_number(Translated *translated, RuntimeState *state);

ArgonObject *new_number_object_from_double(double d);

ArgonObject *new_number_object_from_num_and_den(int64_t n, uint64_t d);

ArgonObject *new_number_object_from_int64(int64_t i64);

void mpq_pow_q(mpq_t rop, const mpq_t e, const mpq_t n);

#endif // RUNTIME_NUMBER_H