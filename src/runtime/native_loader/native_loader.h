/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef native_loader_H
#define native_loader_H
#include "../../arobject.h"

typedef void (*argon_module_init_fn)(
    RuntimeState *,
    ArgonNativeAPI *,
    ArErr *,
    hashmap_GC *
);

ArgonObject *ARGON_LOAD_NATIVE_CODE(size_t argc, ArgonObject **argv, ArErr *err,
                                    RuntimeState *state, ArgonNativeAPI *api);

#endif // native_loader_H