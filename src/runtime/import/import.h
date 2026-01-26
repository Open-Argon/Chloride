/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IMPORT_RUNTIME_H
#define IMPORT_RUNTIME_H
#include "../../arobject.h"

extern ArgonObject*ARGON_IMPORT_FUNCTION;

void runtime_import(Translated *translated, RuntimeState *state,
                    struct Stack *stack, ArErr *err);

#endif // IMPORT_RUNTIME_H