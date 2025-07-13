#include "../object.h"
#include <string.h>
#include "literals.h"

ArgonObject *ARGON_NULL_TYPE = NULL;
ArgonObject *ARGON_NULL = NULL;

ArgonObject *ARGON_BOOL_TYPE = NULL;
ArgonObject *ARGON_TRUE = NULL;
ArgonObject *ARGON_FALSE = NULL;

void init_literals() {
    ARGON_NULL_TYPE = init_argon_class("NULL_TYPE");

    ARGON_NULL = init_child_argon_object(ARGON_NULL_TYPE);
    ARGON_NULL->type=TYPE_NULL;

    ARGON_BOOL_TYPE = init_argon_class("Bool");

    ARGON_FALSE = init_child_argon_object(ARGON_BOOL_TYPE);
    ARGON_FALSE->type=TYPE_BOOL;

    ARGON_TRUE = init_child_argon_object(ARGON_BOOL_TYPE);
    ARGON_TRUE->type=TYPE_BOOL;
}