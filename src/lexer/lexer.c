#include "lex.yy.h"
#include "token.h"
#include "lexer.h"

int lexer() {
    yyscan_t scanner;
    LexerState state = { "file1.src", 1 };

    const char *input = "let x = 10";

    yylex_init(&scanner);

    // Set the extra data *before* scanning
    yyset_extra(&state, scanner);

    void* buffer = yy_scan_string(input, scanner);
    yy_switch_to_buffer(buffer, scanner);

    yylex(scanner);  // This fills the token array

    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);

    // print tokens etc.
    for (int i = 0; i < token_count; i++) {
        printf("Token(type=%d, value='%s')\n", tokens[i].type, tokens[i].value);
    }

    free_tokens();
    return 0;
}