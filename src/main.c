#include "string/string.h"
#include "number/number.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initialize() {
    initNumber();
}

void cleanup() {
    cleanupNumber();
}

int main() {
    initialize();
    char *code = "1.2e20";
    struct number mynum = translateNumber(code);
    if (mynum.denominator == 0) {
        printf("Invalid number\n");
        return 1;
    }
    double f = 1.0 * mynum.numerator / mynum.denominator;
    printf("Numerator: %ld\n", mynum.numerator);
    printf("Denominator: %lu\n", mynum.denominator);
    printf("Float: %lf\n", f);
    return 0;
}
