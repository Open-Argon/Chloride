#ifndef OBJECT_H
#define OBJECT_H
#include "../internals/hashmap/hashmap.h"
#include <gmp.h>
#include <stdbool.h>

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
  struct ArgonObject *cls; // class pointer or type object
  struct hashmap *fields;  // dynamic fields/methods
  union {
    mpq_t as_number;
    bool as_bool;
    char *as_str;
    void *native_fn;
    // others as needed
  } value;
};
#endif // OBJECT_H