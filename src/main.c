#include "string/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char* str = malloc(100 * sizeof(char));
    if (str == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    strcpy(str, "  \t\n\r\f\vHello, World!  \t\n\r\f\v");
    char* clone = cloneString(str);

    if (clone == NULL) {
        printf("Failed to clone string\n");
        return 1;
    }

    stripString(clone, WHITE_SPACE);

    printf("Original string: \"%s\"\n", str);

    free(str);

    
    printf("Cloned string: \"%s\"\n", clone);
    free(clone);

    return 0;
}
