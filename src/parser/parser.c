#include "parser.h"
#include <stddef.h>

TaggedValue parse_token(LinkedList * tokens, size_t *index) {
  Token * token = get_element_at(tokens, *index);
  switch (token->type) {
    case TOKEN_STRING:
      (*index)++;
      return parse_string(*token);
    default:
      perror("unreachable");
      exit(0);
  }
}

void parser(LinkedList * parsed, LinkedList * tokens, bool inline_flag) {
  size_t index = 0;
  size_t length = list_length(tokens);
  while (index < length) {
    TaggedValue parsed_code = parse_token(tokens, &index);
    append(parsed,&parsed_code);
  }
}