#include "object.h"
#include "../../memory.h"
#include "../runtime.h"
#include <string.h>

ArgonObject *BASE_OBJECT = NULL;

void init_base_field() {
    add_field(BASE_OBJECT, "test", BASE_OBJECT);
}

ArgonObject* init_argon_object() {
    ArgonObject *object = ar_alloc(sizeof(ArgonObject));
    object->type = TYPE_OBJECT;
    object->typeObject = NULL;
    object->baseObject = BASE_OBJECT;
    object->fields = createHashmap();
    memset(&object->value, 0, sizeof(object->value));
    return object;
}

void add_field(ArgonObject*target, char* name, ArgonObject *object) {
    hashmap_insert(target->fields, siphash64_bytes(name, strlen(name)),name, object,  0);
}