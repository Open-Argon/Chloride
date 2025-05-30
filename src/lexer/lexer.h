#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "../dynamic_array/darray.h"
#include <stdio.h>

typedef struct {
    const char *path;
    FILE *file;
    int current_column;
    DArray* tokens;
    // add more fields as needed
} LexerState;

void lexer(LexerState state);


#endif // LEXER_H