/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef runtime_access_H
#define runtime_access_H
#include "../objects/literals/literals.h"
#include "../objects/object.h"
#include "../runtime.h"
#include <inttypes.h>

ArgonObject *ARGON_TYPE_TYPE___get_attr__(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state);

#endif // runtime_access_H