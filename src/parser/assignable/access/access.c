#include "access.h"
#include "../../../lexer/token.h"
#include "../../parser.h"
#include <string.h>
#include <stdlib.h>
#include "../../../memory.h"

ParsedValue *parse_access(char*file,DArray *tokens, size_t * index, ParsedValue * to_access) {
  (*index)++;
  error_if_finished(file, tokens, index);
  Token * token = darray_get(tokens, *index);
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedAccess *parsedAccess = checked_malloc(sizeof(ParsedAccess));
  parsedAccess->to_access = to_access;
  parsedAccess->access = strcpy(checked_malloc(strlen(token->value) + 1), token->value);
  parsedValue->type = AST_ACCESS;
  parsedValue->data = parsedAccess;
  (*index)++;
  return parsedValue;
}

void free_parse_access(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedAccess *parsedAccess = parsedValue->data;
  free_parsed(parsedAccess->to_access);
  free(parsedAccess->access);
  free(parsedAccess->to_access);
  free(parsedAccess);
}