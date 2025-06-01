#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "../dynamic_array/darray.h"
#include <stddef.h>

typedef enum { OP_INIT_STRING } OperationType;

typedef struct {
  size_t registerCount;
  DArray bytecode;
} Translated;

#endif