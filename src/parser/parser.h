#include "../lexer/token.h"
#include "string/string.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void parser(LinkedList * parsed, LinkedList * tokens, bool inline_flag);

TaggedValue parse_token(LinkedList * tokens, size_t *index);