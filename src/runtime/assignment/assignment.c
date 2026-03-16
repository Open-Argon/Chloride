/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "assignment.h"
#include "../objects/string/string.h"
#include <stdint.h>

__thread struct hashmap_GC *assignable_keys = NULL;

void runtime_assignment(int64_t length, int64_t offset, int64_t hash,
                        uint8_t from_register, RuntimeState *state,
                        Translated *translated, struct Stack *stack) {
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
  ArgonObject *key = NULL;
  if (assignable_keys)
    key = hashmap_lookup_GC(assignable_keys, hash);
  if (!key) {
    if (!assignable_keys)
      assignable_keys = createHashmap_GC();
    key = new_string_object(data, length, hash);
    hashmap_insert_GC(assignable_keys, hash, NULL, key, 0);
  }
  hashmap_insert_GC(stack->scope, hash, key, state->registers[from_register],
                    0);
}