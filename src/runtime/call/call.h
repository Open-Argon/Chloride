/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef runtime_call_H
#define runtime_call_H
#include "../runtime.h"

ArgonObject *argon_call(ArgonObject *original_object, size_t argc,
                        ArgonObject **argv, ArErr *err, RuntimeState *state);

ArErr run_call(ArgonObject *original_object, size_t argc, ArgonObject **argv,
               RuntimeState *state, bool CStackFrame);

#endif // runtime_call_H