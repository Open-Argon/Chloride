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

ArgonObject *bind_object_to_function(ArgonObject *object,
                                     ArgonObject *function);

ArgonObject *get_field_for_class_l(ArgonObject *target, char *name,
                                   size_t length, ArgonObject *binding_object);

ArgonObject *get_field_l(ArgonObject *target, char *name, size_t length,
                         bool recursive, bool disable_method_wrapper);

ArgonObject *get_field_for_class(ArgonObject *target, char *name,
                                 ArgonObject *binding_object);

ArgonObject *get_field(ArgonObject *target, char *name, bool recursive,
                       bool disable_method_wrapper);

#endif // OBJECT_H