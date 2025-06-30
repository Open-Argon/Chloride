#ifndef RUNTIME_H
#define RUNTIME_H
#include "../translator/translator.h"
#include "internals/hashmap/hashmap.h"

typedef struct {
  ArgonObject **registers;
  size_t head;
} RuntimeState;

typedef struct Stack {
    ArgonObject *scope;
    struct Stack *prev;
} Stack;

void init_types();

uint64_t pop_bytecode(Translated *translated, RuntimeState *state);

void run_instruction(Translated *translated, RuntimeState *state, struct Stack stack);

void runtime(Translated translated);

#endif // RUNTIME_H