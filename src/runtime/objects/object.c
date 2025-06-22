#include "object.h"
#include "../../memory.h"

ArgonObject* init_argon_object() {
    ArgonObject *object = ar_alloc(sizeof(ArgonObject));
    object->type = TYPE_OBJECT;
    object->cls = object;
    object->fields = createHashmap(8);
}