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

void load_number(Translated *translated, RuntimeState *state);

ArgonObject *new_number_object_from_double(double d);

ArgonObject *new_number_object_from_long(long n, unsigned long d);

#endif // RUNTIME_NUMBER_H