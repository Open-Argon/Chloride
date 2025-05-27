%option reentrant

%{
#include "token.h"
#include "lexer.h"
#define GET_STATE LexerState *state = (LexerState *)yyget_extra(yyscanner);
#define GET_ADD_COLUMN COLUMN_NO += yyleng;
#define LINE_NO yylineno+1
#define COLUMN_NO state->current_column

int yywrap(void *) {
    return 1;
}
%}

%%

\"(\\[a-z\"'`]|[^\\"])*\" {
    GET_STATE
    add_token(TOKEN_STRING, yytext, LINE_NO, COLUMN_NO);
    GET_ADD_COLUMN
}

[0-9]+ {
    GET_STATE
    add_token(TOKEN_NUMBER, yytext, LINE_NO, COLUMN_NO);
    GET_ADD_COLUMN
}

[a-zA-Z_][a-zA-Z0-9_]* {
    GET_STATE
    add_token(TOKEN_IDENTIFIER, yytext, LINE_NO, COLUMN_NO);
    GET_ADD_COLUMN
}

"." {
    GET_STATE
    add_token(TOKEN_DOT, yytext, LINE_NO, COLUMN_NO);
    GET_ADD_COLUMN
}

\n {
    GET_STATE
    add_token(TOKEN_NEW_LINE, yytext, LINE_NO, COLUMN_NO);
    COLUMN_NO = 1;
}

[ \t]+ {
    GET_STATE
    GET_ADD_COLUMN  // Advance column for whitespace
}

.   {
    GET_STATE
    fprintf(stderr, "Error in file %s on line %d column %d: unexpected character '%s'\n", state->filename, LINE_NO, COLUMN_NO, yytext);
    exit(1);
}
%%