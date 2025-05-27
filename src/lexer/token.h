#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_DOT,
    TOKEN_NEW_LINE,
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

extern int token_count;

extern Token* tokens;


void add_token(TokenType type, const char* value, int line, int column);

void free_tokens();

#endif