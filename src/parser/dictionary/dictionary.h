#ifndef DICTIONARY_H
#define DICTIONARY_H
#include "../../lexer/token.h" // for Token
#include "../parser.h"

typedef struct {
  ParsedValue * key;
  ParsedValue * value;
} ParsedDictionaryEntry;

ParsedValue *parse_dictionary(char *file, DArray *tokens,
                      size_t *index);

void free_parsed_dictionary(void *ptr);

#endif // DICTIONARY_H