#ifndef NUMBER_H
#define CLONESTRING_HNUMBER_H

#include <stdint.h>
#include <stdlib.h>
    #include <stdbool.h>

struct number {
    int64_t numerator;
    uint64_t denominator;
};

struct number translateNumber(char *code);

void initNumber();
void cleanupNumber();

#endif // NUMBER_H
