#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    const char * path = "test.ar";
    LinkedList* tokens = create_list(sizeof(Token));

    LexerState state = {
        path,
        fopen(path, "r"),
        0,
        tokens
    };
    lexer(state);

    LinkedList * parsed = create_list(sizeof(TaggedValue));

    parser(parsed, tokens, false);

    free_list(tokens, free_token);

    free_list(parsed,free_tagged_value);

    ar_memory_init();

    return 0;
}
