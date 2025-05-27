#include "taggedValue.h"
#include <stdlib.h>

TaggedValueStruct init_TaggedValueStruct() {
    TaggedValueStruct taggedValueStruct = {
        0,
        INITIAL_CAPACITY,
        malloc(sizeof(TaggedValue)*INITIAL_CAPACITY)
    };
    return taggedValueStruct;
}

void TaggedValueStruct_append(TaggedValueStruct *TaggedValueStruct,
                              TaggedValue TaggedValue) {
  if (TaggedValueStruct->count >= TaggedValueStruct->capacity) {
    TaggedValueStruct->capacity *= 2;
    TaggedValueStruct->TaggedValue =
        realloc(TaggedValueStruct->TaggedValue,
                sizeof(TaggedValue) * TaggedValueStruct->capacity);
  }
  TaggedValueStruct[TaggedValueStruct->count].TaggedValue->data =
      TaggedValue.data;
  TaggedValueStruct[TaggedValueStruct->count].TaggedValue->type =
      TaggedValue.type;
  TaggedValueStruct->count++;
}