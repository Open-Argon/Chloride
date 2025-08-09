/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "runtime.h"
#include "../err.h"
#include "../hash_data/hash_data.h"
#include "../translator/translator.h"
#include "call/call.h"
#include "declaration/declaration.h"
#include "internals/hashmap/hashmap.h"
#include "objects/functions/functions.h"
#include "objects/literals/literals.h"
#include "objects/object.h"
#include "objects/string/string.h"
#include "objects/type/type.h"
#include <fcntl.h>
#include <gc/gc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ArgonObject *ARGON_METHOD_TYPE;
Stack *Global_Scope = NULL;

ArgonObject *ARGON_TYPE_TYPE___call__(size_t argc, ArgonObject **argv,
                                      ArErr *err) {
  if (argc<1) {
    *err = create_err(0, 0, 0, "", "Runtime Error", "call missing self object.");
    return ARGON_NULL;
  }
  ArgonObject *self = argv[0];
  ArgonObject *self_name = get_field(argv[0], "__name__", true);

  printf("type: %.*s\n", (int)self_name->value.as_str.length, self_name->value.as_str.data);
  return ARGON_NULL;
}

void bootstrap_types() {
  BASE_CLASS = new_object();
  ARGON_TYPE_TYPE = new_object();
  add_field(ARGON_TYPE_TYPE, "__base__", BASE_CLASS);
  add_field(ARGON_TYPE_TYPE, "__class__", ARGON_TYPE_TYPE);
  add_field(ARGON_TYPE_TYPE, "__call__",
            create_argon_native_function(
                "__call__", ARGON_TYPE_TYPE___call__));

  ARGON_NULL_TYPE = new_object();
  add_field(ARGON_NULL_TYPE, "__base__", BASE_CLASS);
  ARGON_NULL = new_object();
  add_field(ARGON_NULL, "__class__", ARGON_NULL_TYPE);

  add_field(BASE_CLASS, "__base__", ARGON_NULL);

  ARGON_BOOL_TYPE = new_object();
  add_field(ARGON_BOOL_TYPE, "__base__", BASE_CLASS);
  ARGON_TRUE = new_object();
  add_field(ARGON_TRUE, "__class__", ARGON_BOOL_TYPE);
  ARGON_FALSE = new_object();
  add_field(ARGON_FALSE, "__class__", ARGON_BOOL_TYPE);

  ARGON_STRING_TYPE = new_object();
  add_field(ARGON_STRING_TYPE, "__base__", BASE_CLASS);

  add_field(ARGON_STRING_TYPE, "__name__",
            new_string_object_null_terminated("string"));
  add_field(BASE_CLASS, "__name__",
            new_string_object_null_terminated("object"));
  add_field(ARGON_TYPE_TYPE, "__name__",
            new_string_object_null_terminated("type"));
  add_field(ARGON_NULL_TYPE, "__name__",
            new_string_object_null_terminated("null_type"));
  add_field(ARGON_BOOL_TYPE, "__name__",
            new_string_object_null_terminated("boolean"));

  ARGON_FUNCTION_TYPE = new_object();
  add_field(ARGON_FUNCTION_TYPE, "__base__", BASE_CLASS);
  add_field(ARGON_FUNCTION_TYPE, "__name__",
            new_string_object_null_terminated("function"));

  ARGON_METHOD_TYPE = new_object();
  add_field(ARGON_METHOD_TYPE, "__base__", BASE_CLASS);
  add_field(ARGON_METHOD_TYPE, "__name__",
            new_string_object_null_terminated("method"));
}

void add_to_scope(Stack *stack, char *name, ArgonObject *value) {
  size_t length = strlen(name);
  uint64_t hash =
      siphash64_bytes(name, length, siphash_key);
  ArgonObject *key = new_string_object(name, length);
  hashmap_insert_GC(stack->scope, hash, key, value, 0);
}

void bootstrap_globals() {
  Global_Scope = create_scope(NULL);
  add_to_scope(Global_Scope, "string", ARGON_STRING_TYPE);
  add_to_scope(Global_Scope, "type", ARGON_TYPE_TYPE);
}

int compare_by_order(const void *a, const void *b) {
  const struct node_GC *itemA = (const struct node_GC *)a;
  const struct node_GC *itemB = (const struct node_GC *)b;
  return itemA->order - itemB->order;
}

uint8_t pop_byte(Translated *translated, RuntimeState *state) {
  return *((uint8_t *)darray_armem_get(&translated->bytecode, state->head++));
}

uint64_t pop_bytecode(Translated *translated, RuntimeState *state) {
  uint64_t value = 0;
  for (int i = 0; i < 8; i++) {
    value |= ((uint64_t)pop_byte(translated, state)) << (i * 8);
  }
  return value;
}

void load_const(Translated *translated, RuntimeState *state) {
  uint64_t to_register = pop_byte(translated, state);
  types type = pop_byte(translated, state);
  size_t length = pop_bytecode(translated, state);
  uint64_t offset = pop_bytecode(translated, state);

  void *data = ar_alloc_atomic(length);
  memcpy(data, arena_get(&translated->constants, offset), length);
  ArgonObject *object = ARGON_NULL;
  switch (type) {
  case TYPE_OP_STRING:
    object = new_string_object(data, length);
    break;
  }
  state->registers[to_register] = object;
}

struct hashmap *runtime_hash_table = NULL;

uint64_t runtime_hash(const void *data, size_t len, uint64_t prehash) {
  if (!runtime_hash_table) {
    runtime_hash_table = createHashmap();
  } else {
    void *result = hashmap_lookup(runtime_hash_table, prehash);
    if (result) {
      return (uint64_t)result;
    }
  }
  uint64_t hash = siphash64_bytes(data, len, siphash_key);
  hashmap_insert(runtime_hash_table, prehash, 0, (void *)hash, 0);
  return hash;
}

ArErr load_variable(Translated *translated, RuntimeState *state,
                    struct Stack *stack) {
  int64_t length = pop_bytecode(translated, state);
  int64_t offset = pop_bytecode(translated, state);
  uint64_t prehash = pop_bytecode(translated, state);
  uint64_t hash =
      runtime_hash(arena_get(&translated->constants, offset), length, prehash);
  struct Stack *current_stack = stack;
  while (current_stack) {
    ArgonObject *result = hashmap_lookup_GC(current_stack->scope, hash);
    if (result) {
      state->registers[0] = result;
      return no_err;
    }
    current_stack = current_stack->prev;
  }
  ArErr err =
      create_err(state->source_location.line, state->source_location.column,
                 state->source_location.length, state->path, "Name Error",
                 "name '%.*s' is not defined", (int)length,
                 arena_get(&translated->constants, offset));
  return err;
}

ArErr run_instruction(Translated *translated, RuntimeState *state,
                      struct Stack **stack) {
  OperationType opcode = pop_byte(translated, state);
  switch (opcode) {
  case OP_LOAD_NULL:
    state->registers[pop_byte(translated, state)] = ARGON_NULL;
    break;
  case OP_LOAD_CONST:
    load_const(translated, state);
    break;
  case OP_LOAD_FUNCTION:
    load_argon_function(translated, state, *stack);
    break;
  case OP_IDENTIFIER:
    return load_variable(translated, state, *stack);
  case OP_DECLARE:
    return runtime_declaration(translated, state, *stack);
  case OP_BOOL:;
    uint8_t to_register = pop_byte(translated, state);
    state->registers[to_register] = ARGON_TRUE;
    break;
  case OP_JUMP_IF_FALSE:;
    uint8_t from_register = pop_byte(translated, state);
    uint64_t to = pop_bytecode(translated, state);
    if (state->registers[from_register] == ARGON_FALSE) {
      state->head = to;
    }
    break;
  case OP_JUMP:
    state->head = pop_bytecode(translated, state);
    break;
  case OP_NEW_SCOPE:
    *stack = create_scope(*stack);
    break;
  case OP_POP_SCOPE:;
    // struct node_GC *array =
    //     checked_malloc(sizeof(struct node_GC) * (*stack)->scope->count);
    // size_t j = 0;
    // for (size_t i = 0; i < (*stack)->scope->size; i++) {
    //   struct node_GC *temp = (*stack)->scope->list[i];
    //   while (temp) {
    //     array[j++] = *temp;
    //     temp = temp->next;
    //   }
    // }
    // qsort(array, (*stack)->scope->count, sizeof(struct node_GC),
    //       compare_by_order);
    // for (size_t i = 0; i < (*stack)->scope->count; i++) {
    //   struct node_GC temp = array[i];
    //   printf("%.*s = %.*s\n",
    //            (int)((ArgonObject *)temp.key)->value.as_str.length,
    //            ((ArgonObject *)temp.key)->value.as_str.data,
    //            (int)((ArgonObject *)temp.val)->value.as_str.length,
    //            ((ArgonObject *)temp.val)->value.as_str.data);
    // }
    // free(array);
    *stack = (*stack)->prev;
    break;
  case OP_INIT_CALL:;
    size_t length = pop_bytecode(translated, state);
    call_instance call_instance = {
        state->call_instance, state->registers[0],
        checked_malloc(length * sizeof(ArgonObject *)), length};
    state->call_instance = checked_malloc(sizeof(call_instance));
    *state->call_instance = call_instance;
    break;
  case OP_INSERT_ARG:;
    size_t index = pop_bytecode(translated, state);
    (state->call_instance->args)[index] = state->registers[0];
    break;
  case OP_CALL:;
    ArErr err = run_call(translated, state);
    free(state->call_instance->args);
    call_instance = *state->call_instance;
    free(state->call_instance);
    state->call_instance = call_instance.previous;
    return err;
    // ArgonObject *object = state->registers[from_register];
    // char *field = "__class__";
    // uint64_t hash = siphash64_bytes(field, strlen(field), siphash_key);
    // ArgonObject *class = hashmap_lookup_GC(object->dict, hash);
    // field = "__name__";
    // hash = siphash64_bytes(field, strlen(field), siphash_key);
    // ArgonObject *class_name = hashmap_lookup_GC(class->dict, hash);
    // hash = siphash64_bytes(field, strlen(field), siphash_key);
    // ArgonObject *object_name = hashmap_lookup_GC(object->dict, hash);
    // if (object_name) {
    //   printf("call <%.*s %.*s at %p>\n",
    //   (int)class_name->value.as_str.length,
    //   class_name->value.as_str.data,(int)object_name->value.as_str.length,
    //   object_name->value.as_str.data, object);
    // } else {
    //   printf("call <%.*s object at %p>\n",
    //   (int)class_name->value.as_str.length, class_name->value.as_str.data,
    //   object);
    // }
  case OP_SOURCE_LOCATION:
    state->source_location = (SourceLocation){pop_bytecode(translated, state),
                                              pop_bytecode(translated, state),
                                              pop_bytecode(translated, state)};
    break;
  default:
    return create_err(0, 0, 0, NULL, "Runtime Error", "Invalid Opcode %#x",
                      opcode);
  }
  return no_err;
}

RuntimeState init_runtime_state(Translated translated, char *path) {
  RuntimeState runtime = {
      checked_malloc(translated.registerCount * sizeof(ArgonObject *)),
      0,
      path,
      NULL,
      NULL,
      {0, 0, 0},
      {}};
  return runtime;
}

void free_runtime_state(RuntimeState runtime_state) {
  free(runtime_state.registers);
}

Stack *create_scope(Stack *prev) {
  Stack *stack = ar_alloc(sizeof(Stack));
  stack->scope = createHashmap_GC();
  stack->prev = prev;
  return stack;
}

ArErr runtime(Translated translated, RuntimeState state, Stack *stack) {
  state.head = 0;

  StackFrame *currentStackFrame =
      checked_malloc(sizeof(StackFrame) * STACKFRAME_CHUNKS);
  *currentStackFrame = (StackFrame){translated, state, stack, NULL, 0};
  currentStackFrame->state.currentStackFramePointer = &currentStackFrame;
  ArErr err = no_err;
  while (currentStackFrame) {
    while (currentStackFrame->state.head <
               currentStackFrame->translated.bytecode.size &&
           !err.exists) {
      err =
          run_instruction(&currentStackFrame->translated,
                          &currentStackFrame->state, &currentStackFrame->stack);
    }
    StackFrame *tempStackFrame = currentStackFrame;

    while (currentStackFrame->state.call_instance) {
      free(currentStackFrame->state.call_instance->args);
      call_instance call_instance = *currentStackFrame->state.call_instance;
      free(currentStackFrame->state.call_instance);
      currentStackFrame->state.call_instance = call_instance.previous;
    }

    currentStackFrame = currentStackFrame->previousStackFrame;
    if (tempStackFrame->depth % STACKFRAME_CHUNKS == 0) {
      free(tempStackFrame);
    }
  }
  return err;
}