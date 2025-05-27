%{
#include "token.h"
int current_line = 1;
int current_column = 1;

int yywrap() {
    return 1;
}
%}

%%

\"(\\[a-z\"'`]|[^\\"])*\" {
    add_token(TOKEN_STRING, yytext, current_line, current_column);
    current_column += yyleng;
}

[0-9]+ {
    add_token(TOKEN_NUMBER, yytext, current_line, current_column);
    current_column += yyleng;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    add_token(TOKEN_IDENTIFIER, yytext, current_line, current_column);
    current_column += yyleng;
}

"." {
    add_token(TOKEN_DOT, yytext, current_line, current_column);
    current_column += yyleng;
}

\n {
    add_token(TOKEN_NEW_LINE, yytext, current_line, current_column);
    current_line++;
    current_column = 1;
}

[ \t]+ {
    current_column += yyleng;  // Advance column for whitespace
}

.   { 
    fprintf(stderr, "Error: Unexpected character '%c' at line %d\n", *yytext, yylineno);
    exit(1);
}
%%