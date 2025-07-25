/* 
 SPDX-FileCopyrightText: 2025 William Bell

 SPDX-License-Identifier: GPL-3.0-or-later
*/

%option reentrant
%option yylineno

%{
#include "token.h"
#include "lexer.h"
#define GET_STATE LexerState *state = (LexerState *)yyget_extra(yyscanner);
#define COLUMN_NO state->current_column

int yywrap(void * unused_param) {
    (void)unused_param;
    return 1;
}
%}

%%

"\0" {
    fprintf(stderr, "Error: Null character encountered at line %d\n", yylineno);
    exit(1);
}

"."                 { return TOKEN_DOT; }
"!"                 { return TOKEN_EXCLAMATION; }
","                 { return TOKEN_COMMA; }
":"                 { return TOKEN_COLON; }

"="                 { return TOKEN_ASSIGN; }
"+="                { return TOKEN_ASSIGN_PLUS; }
"-="                { return TOKEN_ASSIGN_MINUS; }
"//="               { return TOKEN_ASSIGN_FLOORDIV; }
"/="                { return TOKEN_ASSIGN_SLASH; }
"%="                { return TOKEN_ASSIGN_MODULO; }
"*="                { return TOKEN_ASSIGN_STAR; }
"^="                { return TOKEN_ASSIGN_CARET; }

"not"[ \t]+"in"     { return TOKEN_NOT_IN; }
"in"                { return TOKEN_IN; }
"&&"                { return TOKEN_AND; }
"||"                { return TOKEN_OR; }
"<="                { return TOKEN_LE; }
">="                { return TOKEN_GE; }
"!="                { return TOKEN_NE; }
"=="                { return TOKEN_EQ; }
"//"                { return TOKEN_FLOORDIV; }
"<"                 { return TOKEN_LT; }
">"                 { return TOKEN_GT; }
"+"                 { return TOKEN_PLUS; }
"-"                 { return TOKEN_MINUS; }
"%"                 { return TOKEN_MODULO; }
"*"                 { return TOKEN_STAR; }
"/"                 { return TOKEN_SLASH; }
"^"                 { return TOKEN_CARET; }


"if"                { return TOKEN_IF; }
"else"[ \t]+"if"    { return TOKEN_ELSE_IF; }
"else"              { return TOKEN_ELSE; }
"while"             { return TOKEN_WHILE; }
"forever"           { return TOKEN_FOREVER; }
"for"               { return TOKEN_FOR; }
"break"             { return TOKEN_BREAK; }
"continue"          { return TOKEN_CONTINUE; }
"return"            { return TOKEN_RETURN; }
"let"               { return TOKEN_LET; }
"import"            { return TOKEN_IMPORT; }
"from"              { return TOKEN_FROM; }
"do"                { return TOKEN_DO; }
"true"              { return TOKEN_TRUE; }
"false"             { return TOKEN_FALSE; }
"null"              { return TOKEN_NULL; }
"delete"            { return TOKEN_DELETE; }
"not"               { return TOKEN_NOT; }
"try"               { return TOKEN_TRY; }
"catch"             { return TOKEN_CATCH; }

"("                 { return TOKEN_LPAREN; }
")"                 { return TOKEN_RPAREN; }
"["                 { return TOKEN_LBRACKET; }
"]"                 { return TOKEN_RBRACKET; }
"{"                 { return TOKEN_LBRACE; }
"}"                 { return TOKEN_RBRACE; }

[a-zA-Z_][a-zA-Z0-9_]* { return TOKEN_IDENTIFIER; }

\"((\\([\"\\\/bfnrt]|u[0-9a-fA-F]{4}))|[^\\\"\n])*\" {
    return TOKEN_STRING;
}

\'((\\([\'\\\/bfnrt]|u[0-9a-fA-F]{4}))|[^\\\'\n])*\' {
    return TOKEN_STRING;
}

((([0-9]+(\.[0-9]+)?)|(\.[0-9]+))(e((\-|\+)?([0-9]+(\.[0-9]+)?)))?) {
    return TOKEN_NUMBER;
}

"\n" { return TOKEN_NEW_LINE; }

[ \t]+ {
    GET_STATE
    if (COLUMN_NO == 0){
        return TOKEN_INDENT;
    }
    COLUMN_NO += yyleng;
}

#[^\n]*        { /* skip comment */ }

.   {
    return TOKEN_INVALID;
}
%%