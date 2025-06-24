#ifndef OBJECT_H
#define OBJECT_H
#include "../internals/hashmap/hashmap.h"
#include <gmp.h>
#include <stdbool.h>

extern ArgonObject *BASE_OBJECT;

struct string_struct {
  char *data;
  size_t length;
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
  ArgonObject *baseObject;
  ArgonObject *typeObject;
  struct hashmap *fields; // dynamic fields/methods
  union {
    mpq_t as_number;
    bool as_bool;
    struct string_struct as_str;
    void *native_fn;
    // others as needed
  } value;
};
typedef struct ArgonObject ArgonObject;
void init_base_field();

ArgonObject *init_argon_object();

void add_field(ArgonObject*target, char* name, ArgonObject *object);
#endif // OBJECT_H