#include "lex.yy.h"
#include "token.h"

int lexer() {
    const char *input = "term.log\n";



    void* buffer = yy_scan_string(input);
    yy_switch_to_buffer(buffer);
    yylex();  // This fills the token array
    yy_delete_buffer(buffer);

    for (int i = 0; i < token_count; i++) {
        printf("Token(type=%d, value='%s')\n", tokens[i].type, tokens[i].value);
    }

    free_tokens();
    return 0;
}