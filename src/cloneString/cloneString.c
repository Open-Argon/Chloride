#include "cloneString.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* cloneString(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    size_t len = strlen(str);
    char* clone = malloc((len + 1) * sizeof(char));
    
    if (clone == NULL) {
        return NULL;
    }
    
    strcpy(clone, str);
    return clone;
}
