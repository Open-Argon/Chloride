typedef enum {
  TYPE_STRING,
} ValueType;

typedef struct {
  ValueType type;
  void *data;
  
} TaggedValue;


#define INITIAL_CAPACITY 64


typedef struct {
  int count;
  int capacity;
  TaggedValue * TaggedValue;
} TaggedValueStruct;

TaggedValueStruct init_TaggedValueStruct();
void TaggedValueStruct_append(TaggedValueStruct *TaggedValueStruct,
                              TaggedValue TaggedValue);