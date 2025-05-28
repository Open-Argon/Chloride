#include "../list/list.h"

typedef enum {
  AST_STRING,
} ValueType;

typedef struct {
  ValueType type;
  void *data;
  
} TaggedValue;