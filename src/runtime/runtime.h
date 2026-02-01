/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RUNTIME_H
#define RUNTIME_H
#include "../returnTypes.h"
#include "internals/hashmap/hashmap.h"
#include <stdio.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define MAX_REGISTERS 256

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
  call_instance *call_instance;
  StackFrame **currentStackFramePointer;
  SourceLocation source_location;
  DArray catch_errors; // ErrorCatch[]
  hashmap_GC *load_number_cache;
  char *path;
} RuntimeState;

typedef struct StackFrame {
  Translated translated;
  RuntimeState state;
  Stack *stack;
  StackFrame *previousStackFrame;
  uint64_t depth;
} StackFrame;

void bootstrap_types();

extern struct hashmap_GC *runtime_hash_table;

uint64_t runtime_hash(const void *data, size_t len, uint64_t prehash);

void bootstrap_globals();

static inline void *arena_get(ConstantArena *arena, size_t offset) {
  return (uint8_t *)arena->data + offset;
}

static inline void run_instruction(Translated *translated, RuntimeState *state,
                                   struct Stack **stack, ArErr *err);

void init_runtime_state(RuntimeState *runtime,Translated translated, char *path);

Stack *create_scope(Stack *prev, bool force);

void add_to_scope(Stack *stack, char *name, ArgonObject *value);

void add_to_hashmap(struct hashmap_GC *hashmap, char *name, ArgonObject *value);

void runtime(Translated translated, RuntimeState state, Stack *stack,
             ArErr *err);

#endif // RUNTIME_H