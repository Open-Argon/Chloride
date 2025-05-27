#include "string/string.h"
#include "number/number.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer/lexer.h"

void initialize() {
    initNumber();
}

void cleanup() {
    cleanupNumber();
}

int main() {
    lexer();
    return 0;
}
