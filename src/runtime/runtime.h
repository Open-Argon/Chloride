#ifndef RUNTIME_H
#define RUNTIME_H
#include "../translator/translator.h"
#include "internals/hashmap/hashmap.h"
#include "../returnTypes.h"

typedef struct {
  ArgonObject **registers;
  size_t head;
  char*path;
} RuntimeState;

void init_types();

uint64_t pop_bytecode(Translated *translated, RuntimeState *state);

ArErr run_instruction(Translated *translated, RuntimeState *state,
                      struct Stack stack);

RuntimeState init_runtime_state(Translated translated, char *path);

Stack create_scope(Stack *prev);

ArErr runtime(Translated translated, RuntimeState state, Stack stack);

#endif // RUNTIME_H