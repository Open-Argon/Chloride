#include "../object.h"
#include <stdint.h>
#include <stdio.h>
#include <stdio.h>

ArgonObject *ARGON_STRING_TYPE = NULL;

void init_string_type() {
  ARGON_STRING_TYPE = init_argon_class("String");

}

ArgonObject *init_string_object(char*data, size_t length) {
  ArgonObject * object = init_child_argon_object(ARGON_STRING_TYPE);
  object->type = TYPE_STRING;
  object->value.as_str.data = data;
  object->value.as_str.length = length;
  return object;
}