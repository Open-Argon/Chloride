#include "token.h"

typedef struct {
    const char *path;
    const char *content;
    int current_column;
    TokenStruct* tokens;
    // add more fields as needed
} LexerState;

void lexer(LexerState state);
