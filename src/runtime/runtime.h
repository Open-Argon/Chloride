/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_H
#define RUNTIME_H
#include "../returnTypes.h"
#include "../translator/translator.h"
#include "internals/dynamic_array_armem/darray_armem.h"
#include "internals/hashmap/hashmap.h"

typedef struct StackFrame StackFrame;
typedef struct RuntimeState RuntimeState;

typedef struct ErrorCatch {
  size_t jump_to;
  Stack *stack;
  StackFrame *stackFrame;
} ErrorCatch; 

typedef struct RuntimeState {
  ArgonObject **registers;
  size_t head;
  char *path;
  StackFrame **currentStackFramePointer;
  ArgonObject*** call_args;
  size_t* call_args_length;
  DArray catch_errors; // ErrorCatch[]
} RuntimeState;

typedef struct StackFrame {
  Translated translated;
  RuntimeState state;
  Stack *stack;
  StackFrame *previousStackFrame;
  uint64_t depth;
} StackFrame;

#define STACKFRAME_CHUNKS 64

void bootstrap_types();

extern struct hashmap *runtime_hash_table;

uint64_t runtime_hash(const void *data, size_t len, uint64_t prehash);

uint8_t pop_byte(Translated *translated, RuntimeState *state);

uint64_t pop_bytecode(Translated *translated, RuntimeState *state);

ArErr run_instruction(Translated *translated, RuntimeState *state,
                      struct Stack **stack);

RuntimeState init_runtime_state(Translated translated, char *path);

void free_runtime_state(RuntimeState runtime_state);

Stack *create_scope(Stack *prev);

ArErr runtime(Translated translated, RuntimeState state, Stack *stack);

#endif // RUNTIME_H