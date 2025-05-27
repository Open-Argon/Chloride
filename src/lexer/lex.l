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

(\"(\\[a-z\"'`]|[^\\"])*\") {
    GET_STATE
    add_token(TOKENS,TOKEN_STRING, yytext, LINE_NO, COLUMN_NO);
    ADD_TO_COLUMN
}

('((\\([a-z'\"`]))|[^'])*') {
    GET_STATE
    add_token(TOKENS,TOKEN_STRING, yytext, LINE_NO, COLUMN_NO);
    ADD_TO_COLUMN
}

((([0-9]+(\.[0-9]+)?)|(\.[0-9]+))(e((\-|\+)?([0-9]+(\.[0-9]+)?)))?) {
    GET_STATE
    add_token(TOKENS,TOKEN_NUMBER, yytext, LINE_NO, COLUMN_NO);
    ADD_TO_COLUMN
}

([0-9]+\/[0-9]+) {
    GET_STATE
    add_token(TOKENS,TOKEN_FRACTION, yytext, LINE_NO, COLUMN_NO);
    ADD_TO_COLUMN
}

"not"[ \t]+"in"     { GET_STATE; add_token(TOKENS,TOKEN_NOT_IN, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"&&"                { GET_STATE; add_token(TOKENS,TOKEN_AND, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"||"                { GET_STATE; add_token(TOKENS,TOKEN_OR, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"<="                { GET_STATE; add_token(TOKENS,TOKEN_LE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
">="                { GET_STATE; add_token(TOKENS,TOKEN_GE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"!="                { GET_STATE; add_token(TOKENS,TOKEN_NE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"=="                { GET_STATE; add_token(TOKENS,TOKEN_EQ, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"="                 { GET_STATE; add_token(TOKENS,TOKEN_ASSIGN, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"//"                { GET_STATE; add_token(TOKENS,TOKEN_FLOORDIV, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"<"                 { GET_STATE; add_token(TOKENS,TOKEN_LT, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
">"                 { GET_STATE; add_token(TOKENS,TOKEN_GT, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"+"                 { GET_STATE; add_token(TOKENS,TOKEN_PLUS, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"-"                 { GET_STATE; add_token(TOKENS,TOKEN_MINUS, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"%"                 { GET_STATE; add_token(TOKENS,TOKEN_MODULO, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"*"                 { GET_STATE; add_token(TOKENS,TOKEN_STAR, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"/"                 { GET_STATE; add_token(TOKENS,TOKEN_SLASH, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"^"                 { GET_STATE; add_token(TOKENS,TOKEN_CARET, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }


"if"                { GET_STATE; add_token(TOKENS,TOKEN_IF, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"else"              { GET_STATE; add_token(TOKENS,TOKEN_ELSE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"while"             { GET_STATE; add_token(TOKENS,TOKEN_WHILE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"forever"           { GET_STATE; add_token(TOKENS,TOKEN_FOREVER, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"for"               { GET_STATE; add_token(TOKENS,TOKEN_FOR, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"break"             { GET_STATE; add_token(TOKENS,TOKEN_BREAK, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"continue"          { GET_STATE; add_token(TOKENS,TOKEN_CONTINUE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"return"            { GET_STATE; add_token(TOKENS,TOKEN_RETURN, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"let"               { GET_STATE; add_token(TOKENS,TOKEN_LET, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"import"            { GET_STATE; add_token(TOKENS,TOKEN_IMPORT, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"from"              { GET_STATE; add_token(TOKENS,TOKEN_FROM, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"do"                { GET_STATE; add_token(TOKENS,TOKEN_DO, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"true"              { GET_STATE; add_token(TOKENS,TOKEN_TRUE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"false"             { GET_STATE; add_token(TOKENS,TOKEN_FALSE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"null"              { GET_STATE; add_token(TOKENS,TOKEN_NULL, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"delete"            { GET_STATE; add_token(TOKENS,TOKEN_DELETE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"not"               { GET_STATE; add_token(TOKENS,TOKEN_NOT, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"in"                { GET_STATE; add_token(TOKENS,TOKEN_IN, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"try"               { GET_STATE; add_token(TOKENS,TOKEN_TRY, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"catch"             { GET_STATE; add_token(TOKENS,TOKEN_CATCH, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }

"("                 { GET_STATE; add_token(TOKENS,TOKEN_LPAREN, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
")"                 { GET_STATE; add_token(TOKENS,TOKEN_RPAREN, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"["                 { GET_STATE; add_token(TOKENS,TOKEN_LBRACKET, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"]"                 { GET_STATE; add_token(TOKENS,TOKEN_RBRACKET, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"{"                 { GET_STATE; add_token(TOKENS,TOKEN_LBRACE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }
"}"                 { GET_STATE; add_token(TOKENS,TOKEN_RBRACE, yytext, LINE_NO, COLUMN_NO); ADD_TO_COLUMN; }

[a-zA-Z_][a-zA-Z0-9_]* {
    GET_STATE
    add_token(TOKENS,TOKEN_IDENTIFIER, yytext, LINE_NO, COLUMN_NO);
    ADD_TO_COLUMN
}

"." {GET_STATE;add_token(TOKENS,TOKEN_DOT, yytext, LINE_NO, COLUMN_NO);ADD_TO_COLUMN}
"," {GET_STATE;add_token(TOKENS,TOKEN_COMMA, yytext, LINE_NO, COLUMN_NO);ADD_TO_COLUMN}
":" {GET_STATE;add_token(TOKENS,TOKEN_COLON, yytext, LINE_NO, COLUMN_NO);ADD_TO_COLUMN}

\n {
    GET_STATE
    add_token(TOKENS,TOKEN_NEW_LINE, yytext, LINE_NO, COLUMN_NO);
    COLUMN_NO = 1;
}

[ \t]+ {
    GET_STATE
    if (COLUMN_NO == 1){
        add_token(TOKENS,TOKEN_INDENT, yytext, LINE_NO, COLUMN_NO);
    }
    ADD_TO_COLUMN  // Advance column for whitespace
}

.   {
    GET_STATE
    fprintf(stderr, "%s: line %d column %d: unexpected character '%s'\n", state->path, LINE_NO, COLUMN_NO, yytext);
    exit(1);
}
%%