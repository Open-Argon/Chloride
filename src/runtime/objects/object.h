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
ArgonObject *new_object();

void add_field(ArgonObject *target, char *name, ArgonObject *object);

ArgonObject *get_field(ArgonObject *target, char *name);

#endif // OBJECT_H