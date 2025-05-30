#include "lex.yy.h"
#include "lexer.h"

void lexer(LexerState state) {
    yyscan_t scanner;

    yylex_init(&scanner);

    yyset_extra(&state, scanner);

    void* buffer = yy_scan_string(state.content, scanner);
    yy_switch_to_buffer(buffer, scanner);

    int token;
    while ((token = yylex(scanner)) != 0) {
        Token * token_struct = create_token(
            token,
            yyget_lineno(scanner),
            state.current_column,
            yyget_text(scanner)
        );
        append(state.tokens, token_struct);
        if (token == TOKEN_NEW_LINE) {
            state.current_column = 0;
        } else {
            state.current_column += yyget_leng(scanner);
        }
    }

    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);
}