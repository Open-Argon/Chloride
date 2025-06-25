#include "../../internals/hashmap/hashmap.h"
#include "../object.h"
#include <string.h>
#include "type.h"

ArgonObject *ARGON_TYPE = NULL;

void init_type() {
    ARGON_TYPE = init_argon_class("function");
}