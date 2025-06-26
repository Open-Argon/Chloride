#include "object.h"
#include "../../memory.h"
#include "../runtime.h"
#include <stdbool.h>
#include <string.h>
#include "type/type.h"

ArgonObject *BASE_CLASS = NULL;

void init_base_field() {
    // add_field(BASE_CLASS, "test", BASE_CLASS);
}

ArgonObject* init_child_argon_object(ArgonObject *cls) {
    ArgonObject *object = init_argon_class(NULL);
    object->self = object;
    object->baseObject = cls;
    add_field(object, "__call__", NULL);
    return object;
}


ArgonObject* init_argon_class(char*name) {
    ArgonObject *object = ar_alloc(sizeof(ArgonObject));
    object->name = name;
    object->type = TYPE_OBJECT;
    object->self = NULL;
    object->baseObject = ARGON_TYPE;
    object->fields = createHashmap();
    memset(&object->value, 0, sizeof(object->value));
    return object;
}

void add_field(ArgonObject*target, char* name, ArgonObject *object) {
    hashmap_insert(target->fields, siphash64_bytes(name, strlen(name)),name, object,  0);
}