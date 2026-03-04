/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "assignment.h"
#include <stdint.h>
// #include "../objects/string/string.h"

void runtime_assignment(int64_t length,int64_t offset,int64_t hash,uint8_t from_register,Translated *translated, RuntimeState *state,
                         struct Stack *stack) {
  void *data = arena_get(&translated->constants, offset);
  for (Stack *current_stack = stack; current_stack;
       current_stack = current_stack->prev) {
    ArgonObject *exists = hashmap_lookup_GC(current_stack->scope, hash);
    if (exists) {
      hashmap_insert_GC(current_stack->scope, hash, NULL,
                        state->registers[from_register], 0);
      return;
    }
  }
  // ArgonObject *key = new_string_object(data, length, hash);
  hashmap_insert_GC(stack->scope, hash, NULL, state->registers[from_register],
                    0);
}