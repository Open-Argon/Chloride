/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OBJECT_H
#define OBJECT_H
#include "../internals/hashmap/hashmap.h"
#include "../runtime.h"
#include <stdbool.h>

extern ArgonObject *BASE_CLASS;

typedef struct ArgonObject ArgonObject;
void init_base_field();
ArgonObject *init_child_argon_object(ArgonObject *cls);
ArgonObject *init_argon_class(char *name);

void add_field(ArgonObject *target, char *name, ArgonObject *object);
#endif // OBJECT_H