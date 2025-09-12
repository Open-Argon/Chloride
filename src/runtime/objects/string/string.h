/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STRING_OBJ_H
#define STRING_OBJ_H
#include "../object.h"

extern ArgonObject *ARGON_STRING_TYPE;

void init_string(ArgonObject*object,char *data, size_t length, uint64_t prehash,
                               uint64_t hash);

ArgonObject *new_string_object_without_memcpy(char *data, size_t length, uint64_t prehash,
                               uint64_t hash);

ArgonObject *new_string_object(char *data, size_t length, uint64_t prehash, uint64_t hash);

ArgonObject *new_string_object_null_terminated(char*data);
#endif // STRING_OBJ_H