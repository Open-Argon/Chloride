#include "token.h"
#include "../list/list.h"
#include <stdio.h>

typedef struct {
    const char *path;
    FILE *file;
    int current_column;
    LinkedList* tokens;
    // add more fields as needed
} LexerState;

void lexer(LexerState state);
