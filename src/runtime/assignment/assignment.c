/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "assignment.h"

void runtime_assignment(Translated *translated, RuntimeState *state,
                         struct Stack *stack) {
  int64_t length = pop_bytecode(translated, state);
  int64_t offset = pop_bytecode(translated, state);
  int64_t prehash = pop_bytecode(translated, state);
  int64_t from_register = pop_byte(translated, state);
  void *data = arena_get(&translated->constants, offset);
  uint64_t hash = runtime_hash(data, length, prehash);
  ArgonObject *key = new_string_object(data, length, prehash, hash);
  for (Stack *current_stack = stack; current_stack;
       current_stack = current_stack->prev) {
    ArgonObject *exists = hashmap_lookup_GC(current_stack->scope, hash);
    if (exists) {
      hashmap_insert_GC(current_stack->scope, hash, key,
                        state->registers[from_register], 0);
      return;
    }
  }
  hashmap_insert_GC(stack->scope, hash, key, state->registers[from_register],
                    0);
}