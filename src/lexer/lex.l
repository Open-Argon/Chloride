%option reentrant

%{
#include "token.h"
#include "lexer.h"
#define GET_STATE LexerState *state = (LexerState *)yyget_extra(yyscanner);
#define ADD_TO_COLUMN COLUMN_NO += yyleng;
#define LINE_NO yylineno+1
#define TOKENS state->tokens
#define COLUMN_NO state->current_column

int yywrap(void *) {
    return 1;
}
%}

%%

\"((\\([\"\\\/bfnrt]|u[0-9a-fA-F]{4}))|[^\\\"\n])*\" {
    GET_STATE
    Token * token = create_token(
        TOKEN_STRING,
        LINE_NO,
        COLUMN_NO,
        yytext
    );
    append(TOKENS, token);
    ADD_TO_COLUMN
}

\'((\\([\'\\\/bfnrt]|u[0-9a-fA-F]{4}))|[^\\\'\n])*\' {
    GET_STATE
    append(TOKENS, create_token(
        TOKEN_STRING,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));
    ADD_TO_COLUMN
}

((([0-9]+(\.[0-9]+)?)|(\.[0-9]+))(e((\-|\+)?([0-9]+(\.[0-9]+)?)))?) {
    GET_STATE
    append(TOKENS, create_token(
        TOKEN_NUMBER,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));
    ADD_TO_COLUMN
}

([0-9]+\/[0-9]+) {
    GET_STATE
    append(TOKENS, create_token(
        TOKEN_FRACTION,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));
    ADD_TO_COLUMN
}

"not"[ \t]+"in"     { GET_STATE; append(TOKENS, create_token(TOKEN_NOT_IN,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));
    append(TOKENS, create_token(
        TOKEN_NOT_IN,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"&&"                { GET_STATE; append(TOKENS, create_token(TOKEN_AND,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"||"                { GET_STATE; append(TOKENS, create_token(TOKEN_OR,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"<="                { GET_STATE; append(TOKENS, create_token(TOKEN_LE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
">="                { GET_STATE; append(TOKENS, create_token(TOKEN_GE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"!="                { GET_STATE; append(TOKENS, create_token(TOKEN_NE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"=="                { GET_STATE; append(TOKENS, create_token(TOKEN_EQ,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"="                 { GET_STATE; append(TOKENS, create_token(TOKEN_ASSIGN,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"//"                { GET_STATE; append(TOKENS, create_token(TOKEN_FLOORDIV,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"<"                 { GET_STATE; append(TOKENS, create_token(TOKEN_LT,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
">"                 { GET_STATE; append(TOKENS, create_token(TOKEN_GT,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"+"                 { GET_STATE; append(TOKENS, create_token(TOKEN_PLUS,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"-"                 { GET_STATE; append(TOKENS, create_token(TOKEN_MINUS,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"%"                 { GET_STATE; append(TOKENS, create_token(TOKEN_MODULO,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"*"                 { GET_STATE; append(TOKENS, create_token(TOKEN_STAR,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"/"                 { GET_STATE; append(TOKENS, create_token(TOKEN_SLASH,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"^"                 { GET_STATE; append(TOKENS, create_token(TOKEN_CARET,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }


"if"                { GET_STATE; append(TOKENS, create_token(TOKEN_IF,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"else"              { GET_STATE; append(TOKENS, create_token(TOKEN_ELSE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"while"             { GET_STATE; append(TOKENS, create_token(TOKEN_WHILE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"forever"           { GET_STATE; append(TOKENS, create_token(TOKEN_FOREVER,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"for"               { GET_STATE; append(TOKENS, create_token(TOKEN_FOR,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"break"             { GET_STATE; append(TOKENS, create_token(TOKEN_BREAK,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"continue"          { GET_STATE; append(TOKENS, create_token(TOKEN_CONTINUE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"return"            { GET_STATE; append(TOKENS, create_token(TOKEN_RETURN,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"let"               { GET_STATE; append(TOKENS, create_token(TOKEN_LET,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"import"            { GET_STATE; append(TOKENS, create_token(TOKEN_IMPORT,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"from"              { GET_STATE; append(TOKENS, create_token(TOKEN_FROM,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"do"                { GET_STATE; append(TOKENS, create_token(TOKEN_DO,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"true"              { GET_STATE; append(TOKENS, create_token(TOKEN_TRUE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"false"             { GET_STATE; append(TOKENS, create_token(TOKEN_FALSE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"null"              { GET_STATE; append(TOKENS, create_token(TOKEN_NULL,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"delete"            { GET_STATE; append(TOKENS, create_token(TOKEN_DELETE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"not"               { GET_STATE; append(TOKENS, create_token(TOKEN_NOT,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"in"                { GET_STATE; append(TOKENS, create_token(TOKEN_IN,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"try"               { GET_STATE; append(TOKENS, create_token(TOKEN_TRY,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"catch"             { GET_STATE; append(TOKENS, create_token(TOKEN_CATCH,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }

"("                 { GET_STATE; append(TOKENS, create_token(TOKEN_LPAREN,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
")"                 { GET_STATE; append(TOKENS, create_token(TOKEN_RPAREN,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"["                 { GET_STATE; append(TOKENS, create_token(TOKEN_LBRACKET,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"]"                 { GET_STATE; append(TOKENS, create_token(TOKEN_RBRACKET,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"{"                 { GET_STATE; append(TOKENS, create_token(TOKEN_LBRACE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }
"}"                 { GET_STATE; append(TOKENS, create_token(TOKEN_RBRACE,
        LINE_NO,
        COLUMN_NO,
        yytext
    )); ADD_TO_COLUMN; }

[a-zA-Z_][a-zA-Z0-9_]* {
    GET_STATE
    append(TOKENS, create_token(TOKEN_IDENTIFIER,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));
    ADD_TO_COLUMN
}

"." {GET_STATE;append(TOKENS, create_token(TOKEN_DOT,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));ADD_TO_COLUMN}
"," {GET_STATE;append(TOKENS, create_token(TOKEN_COMMA,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));ADD_TO_COLUMN}
":" {GET_STATE;append(TOKENS, create_token(TOKEN_COLON,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));ADD_TO_COLUMN}

\n {
    GET_STATE
    append(TOKENS, create_token(TOKEN_NEW_LINE,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));
    COLUMN_NO = 1;
}

[ \t]+ {
    GET_STATE
    if (COLUMN_NO == 1){
        append(TOKENS, create_token(TOKEN_INDENT,
        LINE_NO,
        COLUMN_NO,
        yytext
    ));
    }
    ADD_TO_COLUMN  // Advance column for whitespace
}

.   {
    GET_STATE
    fprintf(stderr, "%s: line %d column %d: unexpected character '%s'\n", state->path, LINE_NO, COLUMN_NO, yytext);
    exit(1);
}
%%