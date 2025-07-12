#ifndef IDENTIFIER_H
#define IDENTIFIER_H
#include "../../parser.h"
#include "../../../lexer/token.h"  // for Token

typedef struct {
    char * name;
    size_t line;
    size_t column;
} ParsedIdentifier;

// Function declaration for parsing an identifier
ParsedValueReturn parse_identifier(Token *token);

void free_identifier(void *ptr);

#endif // IDENTIFIER_H