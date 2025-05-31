#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stddef.h>
#include "../dynamic_array/darray.h"


typedef enum {
  OP_INIT_STRING
} OperationType;


typedef struct {
  size_t registerCount;
  DArray bytecode;
} Translated;

#endif