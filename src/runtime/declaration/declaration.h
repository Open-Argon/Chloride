/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef runtime_declaration_H
#define runtime_declaration_H
#include "../runtime.h"

void runtime_declaration(int64_t length,int64_t offset,int64_t prehash,uint8_t from_register,Translated *translated, RuntimeState *state,
                         struct Stack *stack, ArErr *err);

#endif // runtime_declaration_H