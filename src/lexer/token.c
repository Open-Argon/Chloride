#include <stdlib.h>
#include <string.h>
#include "token.h"

#define INITIAL_CAPACITY 64

Token* tokens = NULL;
int token_count = 0;
static int token_capacity = 0;

void add_token(TokenType type, const char* value, int line, int column) {
    if (tokens == NULL) {
        token_capacity = INITIAL_CAPACITY;
        tokens = malloc(sizeof(Token) * token_capacity);
    } else if (token_count >= token_capacity) {
        token_capacity *= 2;
        tokens = realloc(tokens, sizeof(Token) * token_capacity);
    }

    tokens[token_count].type = type;
    tokens[token_count].value = strdup(value);
    tokens[token_count].line = line;
    tokens[token_count].column = column;
    token_count++;
}

void free_tokens() {
    for (int i = 0; i < token_count; ++i) {
        free(tokens[i].value);
    }
    free(tokens);
    tokens = NULL;
    token_count = 0;
    token_capacity = 0;
}