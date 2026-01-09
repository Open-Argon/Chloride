/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STRING_OBJ_H
#define STRING_OBJ_H
#include "../object.h"

extern ArgonObject *ARGON_STRING_TYPE;

char *c_quote_string(const char *input, size_t len);

void init_string(ArgonObject *object, char *data, size_t length,
                 uint64_t prehash, uint64_t hash);

char *argon_object_to_null_terminated_string(ArgonObject *object, ArErr *err,
                                             RuntimeState *state);

ArgonObject *new_string_object_without_memcpy(char *data, size_t length,
                                              uint64_t prehash, uint64_t hash);

ArgonObject *new_string_object(char *data, size_t length, uint64_t prehash,
                               uint64_t hash);

ArgonObject *new_string_object_null_terminated(char *data);

char *argon_string_to_c_string_malloc(ArgonObject *object);
#endif // STRING_OBJ_H