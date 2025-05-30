#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "memory.h"
#include "dynamic_array/darray.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdio.h>

int main() {
    const char * path = "test.ar";
    DArray tokens;

    darray_init(&tokens, sizeof(Token));

    LexerState state = {
        path,
        fopen(path, "r"),
        0,
        &tokens
    };
    lexer(state);

    DArray parsed;

    darray_init(&parsed, sizeof(ParsedValue));


    parser(&parsed, &tokens, false);
    darray_free(&tokens, free_token);

    darray_free(&parsed,free_parsed_value);

    ar_memory_init();

    return 0;
}
