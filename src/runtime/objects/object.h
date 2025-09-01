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

typedef enum {
  __base__,
  __class__,
  __name__,
  __add__,
  __string__,
  __subtract__,
  __multiply__,
  __division__,
  __new__,
  __init__,
  __boolean__,
  __get_attr__,
  __binding__,
  __function__,
  field__address,
  __call__,
  __number__,
  field_log,
  field_length,
  BUILT_IN_FIELDS_COUNT
} built_in_fields;

void init_built_in_field_hashes();

extern ArgonObject *BASE_CLASS;

typedef struct ArgonObject ArgonObject;
ArgonObject *new_object();

void add_builtin_field(ArgonObject *target, built_in_fields field,
                       ArgonObject *object);

void add_field(ArgonObject *target, char *name, uint64_t hash,
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

ArgonObject *get_builtin_field(ArgonObject *target, built_in_fields field,
                               bool recursive, bool disable_method_wrapper);

#endif // OBJECT_H