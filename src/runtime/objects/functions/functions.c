/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../../runtime.h"
#include "../object.h"
#include "../string/string.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

ArgonObject *ARGON_FUNCTION_TYPE = NULL;

ArgonObject *create_argon_native_function(char *name, native_fn native_fn) {
  ArgonObject *object = new_instance(ARGON_FUNCTION_TYPE);
  object->type = TYPE_NATIVE_FUNCTION;
  add_builtin_field(object, __name__,
                    new_string_object(name, strlen(name), 0, 0));
  object->value.native_fn = native_fn;
  return object;
}

void load_argon_function(Translated *translated, RuntimeState *state,
                         struct Stack *stack) {
  ArgonObject *object = new_instance(ARGON_FUNCTION_TYPE);
  object->type = TYPE_FUNCTION;
  uint64_t offset = pop_bytecode(translated, state);
  uint64_t length = pop_bytecode(translated, state);
  add_builtin_field(object, __name__,
                    new_string_object(arena_get(&translated->constants, offset),
                                      length, 0, 0));
  uint64_t number_of_parameters = pop_bytecode(translated, state);
  object->value.argon_fn = ar_alloc(sizeof(struct argon_function_struct)+number_of_parameters *
               sizeof(struct string_struct));
  object->value.argon_fn->parameters = (struct string_struct*)((char*)object->value.argon_fn+sizeof(struct argon_function_struct));
  object->value.argon_fn->translated = *translated;
  object->value.argon_fn->number_of_parameters = number_of_parameters;
  for (size_t i = 0; i < object->value.argon_fn->number_of_parameters; i++) {
    offset = pop_bytecode(translated, state);
    length = pop_bytecode(translated, state);
    object->value.argon_fn->parameters[i].data =
        arena_get(&translated->constants, offset);
    object->value.argon_fn->parameters[i].length = length;
  }
  offset = pop_bytecode(translated, state);
  length = pop_bytecode(translated, state);
  object->value.argon_fn->bytecode = arena_get(&translated->constants, offset);
  object->value.argon_fn->bytecode_length = length;
  object->value.argon_fn->stack = stack;
  state->registers[0] = object;
}