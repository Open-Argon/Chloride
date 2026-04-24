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

ArgonObject *ARGON_STRING_TYPE_get_length(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api);

ArgonObject *ARGON_STRING_TYPE_set_length(size_t argc, ArgonObject **argv,
                                          ArErr *err, RuntimeState *state,
                                          ArgonNativeAPI *api);
ArgonObject *ARGON_STRING_TYPE___equal__(size_t argc, ArgonObject **argv,
                                         ArErr *err, RuntimeState *state,
                                         ArgonNativeAPI *api);
ArgonObject *ARGON_STRING_TYPE___not_equal__(size_t argc, ArgonObject **argv,
                                             ArErr *err, RuntimeState *state,
                                             ArgonNativeAPI *api);
ArgonObject *ARGON_STRING_TYPE___less_than__(size_t argc, ArgonObject **argv,
                                             ArErr *err, RuntimeState *state,
                                             ArgonNativeAPI *api);
ArgonObject *ARGON_STRING_TYPE___less_than_equal__(size_t argc,
                                                   ArgonObject **argv,
                                                   ArErr *err,
                                                   RuntimeState *state,
                                                   ArgonNativeAPI *api);
ArgonObject *ARGON_STRING_TYPE___greater_than__(size_t argc, ArgonObject **argv,
                                                ArErr *err, RuntimeState *state,
                                                ArgonNativeAPI *api);
ArgonObject *ARGON_STRING_TYPE___greater_than_equal__(size_t argc,
                                                      ArgonObject **argv,
                                                      ArErr *err,
                                                      RuntimeState *state,
                                                      ArgonNativeAPI *api);

ArgonObject *ARGON_STRING_TYPE_split(size_t argc, ArgonObject **argv,
                                     ArErr *err, RuntimeState *state,
                                     ArgonNativeAPI *api);

void init_small_chars();
#endif // STRING_OBJ_H