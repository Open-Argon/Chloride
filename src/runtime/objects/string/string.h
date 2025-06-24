#ifndef STRING_OBJ_H
#define STRING_OBJ_H
#include "../object.h"

extern ArgonObject *ARGON_STRING_TYPE;

void init_string_type();

ArgonObject *init_string_object(char*data, size_t length);

#endif // STRING_OBJ_H