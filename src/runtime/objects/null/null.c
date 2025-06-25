#include "../../internals/hashmap/hashmap.h"
#include "../object.h"
#include <string.h>
#include "null.h"

ArgonObject *ARGON_NULL_TYPE = NULL;
ArgonObject *ARGON_NULL = NULL;

void init_null() {
    ARGON_NULL_TYPE = init_argon_class("NULL_TYPE");

    ARGON_NULL = init_child_argon_object(ARGON_NULL_TYPE);
    ARGON_NULL->type=TYPE_NULL;
}