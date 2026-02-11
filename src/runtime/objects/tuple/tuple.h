/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TUPLE_OBJECT
#define TUPLE_OBJECT
#include "../object.h"

extern ArgonObject *ARGON_TUPLE_TYPE;
extern ArgonObject *ARGON_TUPLE_CREATE;

void init_tuple_type();

#endif // TUPLE_OBJECT