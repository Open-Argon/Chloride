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

ArgonObject *ARRAY_CREATE(size_t argc, ArgonObject **argv, ArErr *err,
                          RuntimeState *state, ArgonNativeAPI *api);

EXPOSE_ARGON_METHOD(ARRAY_ITERATOR_TYPE, __next__)

#endif // ARRAY_OBJECT