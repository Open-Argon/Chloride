/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_RANGE_ITERATOR_H
#define RUNTIME_RANGE_ITERATOR_H
#include "../object.h"

extern ArgonObject *ARGON_RANGE_ITERATOR_TYPE;

void init_range_iterator();

ArgonObject *ARGON_RANGE_ITERATOR_TYPE___next__(size_t argc, ArgonObject **argv,
                                                ArErr *err, RuntimeState *state,
                                                ArgonNativeAPI *api);


#endif // RUNTIME_RANGE_ITERATOR_H