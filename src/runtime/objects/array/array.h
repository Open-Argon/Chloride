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

ArgonObject *ARGON_ARRAY_ITERATOR___next__(size_t argc, ArgonObject **argv,
                                           ArErr *err, RuntimeState *state,
                                           ArgonNativeAPI *api);

#endif // ARRAY_OBJECT