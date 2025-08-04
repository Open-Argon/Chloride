/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../../runtime.h"
#include "../object.h"
#include "../string/string.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

ArgonObject *ARGON_FUNCTION_TYPE = NULL;

void load_argon_function(Translated *translated, RuntimeState *state,
                                 struct Stack *stack) {
  ArgonObject *object = new_object();
  add_field(object, "__class__", ARGON_FUNCTION_TYPE);
  object->type = TYPE_FUNCTION;
  uint64_t offset = pop_bytecode(translated, state);
  uint64_t length = pop_bytecode(translated, state);
  add_field(object, "__name__", new_string_object(arena_get(&translated->constants, offset), length));
  object->value.argon_fn.path = translated->path;
  object->value.argon_fn.number_of_parameters = pop_bytecode(translated, state);
  object->value.argon_fn.parameters =
      ar_alloc(object->value.argon_fn.number_of_parameters * sizeof(struct string_struct));
  for (size_t i = 0; i < object->value.argon_fn.number_of_parameters; i++) {
    offset = pop_bytecode(translated, state);
    length = pop_bytecode(translated, state);
    object->value.argon_fn.parameters[i].data = arena_get(&translated->constants, offset);
    object->value.argon_fn.parameters[i].length = length;
  }
  offset = pop_bytecode(translated, state);
  length = pop_bytecode(translated, state);
  object->value.argon_fn.bytecode = arena_get(&translated->constants, offset);
  object->value.argon_fn.bytecode_length = length;
  object->value.argon_fn.stack = stack;
  state->registers[0]=object;
}