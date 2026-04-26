/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STRING_OBJ_H
#define STRING_OBJ_H
#include "../object.h"

extern ArgonObject *ARGON_STRING_TYPE;

extern ArgonObject *ARGON_RENDER_TEMPLATE;

extern ArgonObject *ARGON_STRING_ITERATOR_TYPE;

ArgonObject *RENDER_TEMPLATE(size_t argc, ArgonObject **argv, ArErr *err,
                             RuntimeState *state, ArgonNativeAPI *api);

char *c_quote_string(const char *input, size_t len);

void init_string(ArgonObject *object, char *data, size_t length, uint64_t hash);

char *argon_object_to_length_terminated_string_from___string__(
    ArgonObject *object, ArErr *err, RuntimeState *state, size_t *length);

char *argon_object_to_null_terminated_string(ArgonObject *object, ArErr *err,
                                             RuntimeState *state);

ArgonObject *new_string_object_without_memcpy(char *data, size_t length,
                                              uint64_t hash);

ArgonObject *new_string_object(char *data, size_t length, uint64_t hash);

ArgonObject *new_string_object_null_terminated(char *data);

char *argon_string_to_c_string_malloc(ArgonObject *object);

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, get_length)

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, set_length)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __getitem__)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __equal__)

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __not_equal__)

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __less_than__)

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __less_than_equal__)

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __greater_than__)

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __greater_than_equal__)

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, split)

EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __iter__)
EXPOSE_ARGON_METHOD(ARGON_STRING_ITERATOR_TYPE, __next__)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, chr)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, ord)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, __contains__)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, upper)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, lower)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, title)
EXPOSE_ARGON_METHOD(ARGON_STRING_TYPE, replace)

void init_small_chars();
#endif // STRING_OBJ_H