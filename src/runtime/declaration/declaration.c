/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "declaration.h"
#include "../../err.h"
#include "../assignment/assignment.h"
#include "../objects/string/string.h"
#include "../objects/exceptions/exceptions.h"
#include <stdint.h>

void runtime_declaration(int64_t length, int64_t offset, int64_t hash,
                         uint8_t from_register, Translated *translated,
                         RuntimeState *state, struct Stack *stack, ArErr *err) {
  void *data = arena_get(&translated->constants, offset);
  ArgonObject *exists = hashmap_lookup_GC(stack->scope, hash);
  if (exists) {
    *err = path_specific_create_err(
        state->source_location.line, state->source_location.column,
        state->source_location.length, state->path, RuntimeError,
        "Identifier '%.*s' has already been declared in the current scope",
        length, arena_get(&translated->constants, offset));
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