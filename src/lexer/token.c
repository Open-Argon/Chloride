#include "token.h"
#include <stdio.h>
#include <stdlib.h>

Token *create_token(TokenType type, int line, int column, char *value) {
    Token * token = malloc(sizeof(Token));
    printf("%s\n", value);
    token->type = type;
    token->line=line;
    token->column=column;
    token->value=value;
    return token;
}