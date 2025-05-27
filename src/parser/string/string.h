#include "../../lexer/token.h"
#include "../taggedValue.h"

char *swap_quotes(const char *input);

char *unquote(const char *str);

TaggedValue parse_string(Token token);