#include <stdlib.h>
#include <string.h>
#include "token.h"

#define INITIAL_CAPACITY 64


TokenStruct* init_token() {
    TokenStruct *tokenStruct = malloc(sizeof(TokenStruct));\
    if (tokenStruct == NULL) {
        // handle malloc failure
        return NULL;
    }
    tokenStruct->count = 0;
    tokenStruct->capacity = INITIAL_CAPACITY;
    tokenStruct->tokens = malloc(sizeof(Token) * INITIAL_CAPACITY);
    if (tokenStruct->tokens == NULL) {
        // handle malloc failure
        free(tokenStruct);
        return NULL;
    }
    return tokenStruct;
}

void add_token(TokenStruct* token,TokenType type, const char* value, int line, int column) {
    if (token->count >= token->capacity) {
        token->capacity *= 2;
        token->tokens = realloc(token->tokens, sizeof(Token) * token->capacity);
    }

    token->tokens[token->count].type = type;
    token->tokens[token->count].value = strdup(value);
    token->tokens[token->count].line = line;
    token->tokens[token->count].column = column;
    token->count++;
}

void free_tokens(TokenStruct* token) {
    for (int i = 0; i < token->count; ++i) {
        free(token->tokens[i].value);
    }
    free(token->tokens);
    token->tokens = NULL;
    token->count = 0;
    token->capacity = 0;
    free(token);
}