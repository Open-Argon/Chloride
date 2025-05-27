#include "lex.yy.h"
#include "lexer.h"
#include "../string/string.h"
#include <stdlib.h>

void lexer(LexerState state) {
    yyscan_t scanner;

    char *unquoted = unquote(state.content);
    if (unquoted) {
        printf("%s\n", unquoted);
        free(unquoted);
    }

    yylex_init(&scanner);

    yyset_extra(&state, scanner);

    void* buffer = yy_scan_string(state.content, scanner);
    yy_switch_to_buffer(buffer, scanner);

    yylex(scanner);

    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);
}