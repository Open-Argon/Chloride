#include "../../internals/hashmap/hashmap.h"
#include "../object.h"
#include <string.h>
#include "null.h"

ArgonObject *ARGON_NULL = NULL;

void init_null() {
    ARGON_NULL = init_argon_object();
}