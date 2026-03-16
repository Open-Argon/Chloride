/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef runtime_assignment_H
#define runtime_assignment_H
#include "../runtime.h"

extern struct hashmap_GC *assignable_keys;

void runtime_assignment(int64_t length, int64_t offset, int64_t hash,
                        uint8_t from_register, RuntimeState *state,
                        Translated *translated, struct Stack *stack);

#endif // runtime_assignment_H