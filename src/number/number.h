#ifndef NUMBER_H
#define CLONESTRING_HNUMBER_H

#include <stdint.h>

struct number {
    uint8_t sign;
    uint64_t numerator;
    uint64_t denominator;
};

struct number translateNumber(char *code);

#endif // NUMBER_H
