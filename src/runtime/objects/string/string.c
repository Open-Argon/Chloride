#include "../object.h"
#include <stdint.h>

ArgonObject *ARGON_STRING_TYPE = NULL;

void init_string_type() {
  ARGON_STRING_TYPE = init_argon_object();
}

ArgonObject *init_string_object(char*data, size_t length) {
  ArgonObject * object = init_argon_object();
  object->typeObject = ARGON_STRING_TYPE;
  object->value.as_str.data = data;
  object->value.as_str.length = length;
  return object;
}