#include "../../runtime.h"
#include "../object.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

ArgonObject *ARGON_FUNCTION_TYPE = NULL;

void init_function_type() {
  ARGON_FUNCTION_TYPE = init_argon_class("Function");
}

void load_argon_function(Translated *translated, RuntimeState *state,
                                 struct Stack stack) {
  ArgonObject *object = init_child_argon_object(ARGON_FUNCTION_TYPE);
  object->type = TYPE_FUNCTION;
  uint64_t offset = pop_bytecode(translated, state);
  uint64_t length = pop_bytecode(translated, state);
  object->name = ar_alloc_atomic(length + 1);
  memcpy(object->name, arena_get(&translated->constants, offset), length);
  object->name[length] = '\0';
  object->value.argon_fn.number_of_parameters = pop_bytecode(translated, state);
  object->value.argon_fn.parameters =
      ar_alloc(object->value.argon_fn.number_of_parameters * sizeof(char *));
  printf("%s(", object->name);
  for (size_t i = 0; i < object->value.argon_fn.number_of_parameters; i++) {
    if(i!=0)printf(", ");
    offset = pop_bytecode(translated, state);
    length = pop_bytecode(translated, state);
    object->value.argon_fn.parameters[i] = ar_alloc_atomic(length + 1);
    memcpy(object->value.argon_fn.parameters[i], arena_get(&translated->constants, offset), length);
    object->value.argon_fn.parameters[i][length] = '\0';
    printf("%s", object->value.argon_fn.parameters[i]);
  }
  printf(") = ");
  offset = pop_bytecode(translated, state);
  length = pop_bytecode(translated, state);
  darray_init(&object->value.argon_fn.bytecode, sizeof(uint64_t));
  darray_resize(&object->value.argon_fn.bytecode, length/object->value.argon_fn.bytecode.element_size);
  memcpy(object->value.argon_fn.bytecode.data, arena_get(&translated->constants, offset), length);
  object->value.argon_fn.stack = stack;
  fwrite(object->value.argon_fn.bytecode.data, object->value.argon_fn.bytecode.element_size, object->value.argon_fn.bytecode.size, stdout);
  printf("\n");
  state->registers[0]=object;
}