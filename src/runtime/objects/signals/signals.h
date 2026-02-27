/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_SIGNALS_H
#define RUNTIME_SIGNALS_H
#include "../../../arobject.h"

extern ArgonObject*SIGNAL_CLASS;
extern ArgonObject*END_ITERATION;

void init_signals();

#endif // RUNTIME_SIGNALS_H