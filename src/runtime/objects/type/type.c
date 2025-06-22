#include "../object.h"
#include "../../internals/hashmap/hashmap.h"
#include "../../../memory.h"
#include <string.h>
#include "type.h"

ArgonObject *ARGON_TYPE_OBJ = NULL;

void init_type_obj() {
    ARGON_TYPE_OBJ = ar_alloc(sizeof(ArgonObject));
    ARGON_TYPE_OBJ->type = TYPE_OBJECT;
    ARGON_TYPE_OBJ->cls = ARGON_TYPE_OBJ;
    ARGON_TYPE_OBJ->fields = createHashmap(8);
    memset(&ARGON_TYPE_OBJ->value, 0, sizeof(ARGON_TYPE_OBJ->value));
}