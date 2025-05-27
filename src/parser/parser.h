#include "../lexer/token.h"
#include "string/string.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void parser(TaggedValueStruct * TaggedValueStruct, TokenStruct * tokenStruct, bool inline_flag);

TaggedValue parse_token(TokenStruct * tokenStruct, int *index);