#ifndef LIST_H
#define LIST_H
#include "../../lexer/token.h" // for Token
#include "../parser.h"

ParsedValue *parse_list(char *file, DArray *tokens,
                      size_t *index);

void free_parsed_list(void *ptr);

#endif // LIST_H