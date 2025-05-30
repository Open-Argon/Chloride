#include "parser.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "../lexer/token.h"
#include "../list/list.h"
#include "string/string.h"

TaggedValue * parse_token(LinkedList * tokens, size_t *index) {
  Token * token = get_element_at(tokens, *index);
  switch (token->type) {
    case TOKEN_STRING:
      (*index)++;
      return parse_string(*token);
    case TOKEN_NEW_LINE:
      (*index)++;
      return NULL;
    default:
        fprintf(stderr, "Panic: %s\n", "unreachable"); \
        exit(EXIT_FAILURE);            \
  }
}

void parser(LinkedList * parsed, LinkedList * tokens, bool inline_flag) {
  size_t index = 0;
  size_t length = list_length(tokens);
  while (index < length) {
    TaggedValue * parsed_code = parse_token(tokens, &index);
    if (parsed_code)
      append(parsed,parsed_code);
  }
}

void free_tagged_value(void *ptr) {
    TaggedValue *tagged = ptr;
    switch (tagged->type) {
        case AST_STRING:
            free(tagged->data);
            break;
        // Add cases if needed
    }
    free(tagged);  // Always free the TaggedValue itself
}