#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "memory.h"
#include "dynamic_array/darray.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int main() {
    ar_memory_init();
    char * path = "test.ar";
    DArray tokens;

    darray_init(&tokens, sizeof(Token));

    LexerState state = {
        path,
        fopen(path, "r"),
        0,
        &tokens
    };
    lexer(state);
    fclose(state.file);

    DArray ast;

    darray_init(&ast, sizeof(ParsedValue));


    parser(path,&ast, &tokens, false);
    darray_free(&tokens, free_token);
    
    darray_free(&ast,free_parsed);

    return 0;
}
