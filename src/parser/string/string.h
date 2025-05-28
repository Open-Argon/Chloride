#include "../../lexer/token.h"
#include "../taggedValue.h"

char *swap_quotes(char *input);

char *unquote(char *str);

TaggedValue parse_string(Token token);