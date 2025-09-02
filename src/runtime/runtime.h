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
#include <stdio.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

extern ArgonObject *ARGON_METHOD_TYPE;
extern Stack *Global_Scope;

typedef struct StackFrame StackFrame;
typedef struct RuntimeState RuntimeState;

typedef struct SourceLocation {
  uint64_t line;
  uint64_t column;
  uint64_t length;
} SourceLocation;

typedef struct call_instance {
  struct call_instance *previous;
  ArgonObject *to_call;
  ArgonObject **args;
  size_t args_length;
} call_instance;

typedef struct ErrorCatch {
  size_t jump_to;
  Stack *stack;
  StackFrame *stackFrame;
} ErrorCatch;

typedef struct RuntimeState {
  ArgonObject **registers;
  size_t head;
  char *path;
  call_instance *call_instance;
  StackFrame **currentStackFramePointer;
  SourceLocation source_location;
  DArray catch_errors; // ErrorCatch[]
} RuntimeState;

typedef struct StackFrame {
  Translated translated;
  RuntimeState state;
  Stack *stack;
  StackFrame *previousStackFrame;
  uint64_t depth;
} StackFrame;

void bootstrap_types();

extern struct hashmap *runtime_hash_table;

uint64_t runtime_hash(const void *data, size_t len, uint64_t prehash);

void bootstrap_globals();

static inline void *arena_get(ConstantArena *arena, size_t offset) {
  return (char *)arena->data + offset;
}

static inline uint8_t pop_byte(Translated *translated, RuntimeState *state) {
  return *(((uint8_t *)(translated->bytecode.data)) + state->head++);
}

static inline uint64_t pop_bytecode(Translated *translated,
                                    RuntimeState *state) {
  uint64_t *ptr =
      (uint64_t *)((uint8_t *)translated->bytecode.data + state->head);
  state->head += 8;
  return *ptr;
}

static inline void run_instruction(Translated *translated, RuntimeState *state,
                                   struct Stack **stack, ArErr *err);

RuntimeState init_runtime_state(Translated translated, char *path);

Stack *create_scope(Stack *prev);

void runtime(Translated translated, RuntimeState state, Stack *stack,
             ArErr *err);

#endif // RUNTIME_H