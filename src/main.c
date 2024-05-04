#include "cloneString/cloneString.h"

#include <stdio.h>
#include <stdlib.h>

int main() {
    const char* str = "Hello, World!";
    char* clone = cloneString(str);

    if (clone == NULL) {
        printf("Failed to clone string\n");
        return 1;
    }

    printf("Original string: %s\n", str);
    printf("Cloned string: %s\n", clone);

    free(clone);

    return 0;
}