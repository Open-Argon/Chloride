/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef runtime_declaration_H
#define runtime_declaration_H
#include "../runtime.h"
#include "../objects/string/string.h"

ArErr runtime_declaration(Translated *translated, RuntimeState *state,
                          struct Stack *stack);

#endif // runtime_declaration_H