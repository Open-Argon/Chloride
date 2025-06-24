#include "../object.h"
#include <stdint.h>
#include <stdio.h>
#include <stdio.h>

ArgonObject *ARGON_STRING_TYPE = NULL;
ArgonObject *ARGON_STRING_BASE = NULL;

void init_string_type() {
  ARGON_STRING_TYPE = init_argon_object();

  ARGON_STRING_BASE = init_argon_object();

}

ArgonObject *init_string_object(char*data, size_t length) {
  fwrite(data, 1, length, stdout);
  printf("\n");
  ArgonObject * object = init_argon_object();
  object->typeObject = ARGON_STRING_TYPE;
  object->baseObject = ARGON_STRING_BASE;
  object->value.as_str.data = data;
  object->value.as_str.length = length;
  return object;
}