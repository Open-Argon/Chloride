#include "parser.h"

TaggedValue parse_token(TokenStruct * tokenStruct, int *index) {
  Token token = tokenStruct->tokens[*index];
  switch (token.type) {
    case TOKEN_STRING:
      index++;
      return parse_string(token);
    default:
      perror("unreachable");
      exit(0);
  }
}

void parser(TaggedValueStruct * taggedValueStruct, TokenStruct * tokenStruct, bool inline_flag) {
  int index = 0;
  while (index < tokenStruct->count) {
    TaggedValueStruct_append(taggedValueStruct, parse_token(tokenStruct, &index));
  }
}