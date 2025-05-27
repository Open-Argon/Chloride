#include "number/number.h"
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
