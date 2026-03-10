#ifndef Argon_NATIVE_TYPES_H
#define Argon_NATIVE_TYPES_H

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif

typedef enum ArgonType {
  TYPE_NULL,
  TYPE_BOOL,
  TYPE_NUMBER,
  TYPE_STRING,
  TYPE_FUNCTION,
  TYPE_NATIVE_FUNCTION,
  TYPE_METHOD,
  TYPE_DICTIONARY,
  TYPE_ARRAY,
  TYPE_TUPLE,
  TYPE_BUFFER,
  TYPE_ERROR,
  TYPE_OBJECT,
  TYPE_RANGE_ITERATOR,
  TYPE_ARRAY_ITERATOR,
  TYPE_DICTIONARY_ITERATOR,
} ArgonType;

#ifdef __cplusplus
}
#endif

#endif // Argon_NATIVE_TYPES_H