#include "lex.yy.h"
#include "lexer.h"

void lexer(LexerState state) {
    yyscan_t scanner;

    yylex_init(&scanner);

    yyset_extra(&state, scanner);

    void* buffer = yy_scan_string(state.content, scanner);
    yy_switch_to_buffer(buffer, scanner);

    yylex(scanner);

    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);
}