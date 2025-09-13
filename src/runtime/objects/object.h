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
#include <string.h>

extern ArgonObject *BASE_CLASS;

typedef struct ArgonObject ArgonObject;

ArgonObject *new_class();
ArgonObject *new_instance(ArgonObject * of);

void init_built_in_field_hashes();

int64_t hash_object(ArgonObject *object, ArErr *err, RuntimeState *state);

void add_builtin_field(ArgonObject *target, built_in_fields field,
                       ArgonObject *object);

void add_field_l(ArgonObject *target, char *name, uint64_t hash, size_t length,
                 ArgonObject *object);

ArgonObject *bind_object_to_function(ArgonObject *object,
                                     ArgonObject *function);

ArgonObject *get_field_for_class_l(ArgonObject *target, char *name,
                                   uint64_t hash, size_t length,
                                   ArgonObject *binding_object);

ArgonObject *get_field_l(ArgonObject *target, char *name, uint64_t hash,
                         size_t length, bool recursive,
                         bool disable_method_wrapper);

ArgonObject *get_builtin_field_for_class(ArgonObject *target,
                                         built_in_fields field,
                                         ArgonObject *binding_object);

ArgonObject *get_builtin_field(ArgonObject *target, built_in_fields field);

ArgonObject *get_builtin_field_with_recursion_support(ArgonObject *target, built_in_fields field, bool recursive, bool disable_method_wrapper);

#endif // OBJECT_H