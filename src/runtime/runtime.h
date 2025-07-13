/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_H
#define RUNTIME_H
#include "../translator/translator.h"
#include "internals/hashmap/hashmap.h"
#include "../returnTypes.h"

typedef struct {
  ArgonObject **registers;
  size_t head;
  char*path;
  ArgonObject * return_value;
} RuntimeState;

void init_types();

extern struct hashmap * runtime_hash_table;

uint64_t runtime_hash(const void *data, size_t len, uint64_t prehash);

uint8_t pop_byte(Translated *translated, RuntimeState *state);

uint64_t pop_bytecode(Translated *translated, RuntimeState *state);

ArErr run_instruction(Translated *translated, RuntimeState *state,
                      struct Stack **stack);

RuntimeState init_runtime_state(Translated translated, char *path);

Stack *create_scope(Stack *prev);

ArErr runtime(Translated translated, RuntimeState state, Stack *stack);

#endif // RUNTIME_H