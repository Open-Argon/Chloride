// parser.h

#ifndef iF_H
#define iF_H
#include "../parser.h"
#include "../../lexer/token.h"  // for Token

typedef struct {
  DArray * condition;
  ParsedValue * content;
} ParsedConditional;

ParsedValue *parse_if(char *file, DArray *parsed, DArray *tokens, size_t *index);

void free_parsed_if(void *ptr);


#endif // iF_H