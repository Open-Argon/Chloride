/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef runtime_term_H
#define runtime_term_H
#include "../../objects/literals/literals.h"
#include "../../objects/object.h"
#include "../../runtime.h"

ArgonObject *term_log(size_t argc, ArgonObject **argv, ArErr *err,
                      RuntimeState *state, ArgonNativeAPI *api);

#endif // runtime_term_H