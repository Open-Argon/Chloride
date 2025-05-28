#include "token.h"
#include "../list/list.h"

typedef struct {
    const char *path;
    const char *content;
    int current_column;
    LinkedList* tokens;
    // add more fields as needed
} LexerState;

void lexer(LexerState state);
