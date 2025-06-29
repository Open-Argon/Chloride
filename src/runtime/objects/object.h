#ifndef OBJECT_H
#define OBJECT_H
#include "../internals/hashmap/hashmap.h"
#include "../internals/dynamic_array_armem/darray_armem.h"
#include <gmp.h>
#include <stdbool.h>
#include "../runtime.h"

extern ArgonObject *BASE_CLASS;

struct string_struct {
  char *data;
  size_t length;
};
struct argon_function_struct {
  darray_armem bytecode;
  struct Stack stack;
  size_t number_of_parameters;
  char** parameters;
};

typedef enum {
  TYPE_NULL,
  TYPE_BOOL,
  TYPE_NUMBER,
  TYPE_STRING,
  TYPE_FUNCTION,
  TYPE_NATIVE_FUNCTION,
  TYPE_OBJECT, // generic user object
} ArgonType;

struct ArgonObject {
  ArgonType type;
  char* name;
  ArgonObject *self;
  ArgonObject *baseObject;
  struct hashmap *fields; // dynamic fields/methods
  union {
    mpq_t as_number;
    bool as_bool;
    struct string_struct as_str;
    void *native_fn;
    struct argon_function_struct argon_fn;
    // others as needed
  } value;
};
typedef struct ArgonObject ArgonObject;
void init_base_field();
ArgonObject* init_child_argon_object(ArgonObject *cls);
ArgonObject* init_argon_class(char*name);

void add_field(ArgonObject*target, char* name, ArgonObject *object);
#endif // OBJECT_H