/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARRAY_OBJECT
#define ARRAY_OBJECT
#include "../object.h"

extern ArgonObject *ARRAY_TYPE;
extern ArgonObject *ARGON_ARRAY_CREATE;

void init_array_type();

#endif // ARRAY_OBJECT