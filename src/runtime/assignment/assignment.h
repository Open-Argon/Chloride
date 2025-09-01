/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef runtime_assignment_H
#define runtime_assignment_H
#include "../runtime.h"
#include "../objects/string/string.h"

void runtime_assignment(Translated *translated, RuntimeState *state,
                         struct Stack *stack);

#endif // runtime_assignment_H