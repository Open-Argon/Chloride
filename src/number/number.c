#include "number.h"

#include <string.h>

struct number translateNumber(char *code) {
    struct number num;
    num.sign = code[0] == '-' ? 1 : 0;
    return num;
}
