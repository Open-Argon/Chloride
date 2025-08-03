/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FUNCTION_H
#define FUNCTION_H
#include "../object.h"

extern ArgonObject *ARGON_FUNCTION_TYPE;

ArgonObject *load_argon_function(Translated *translated, RuntimeState *state, struct Stack *stack);

#endif // FUNCTION_H