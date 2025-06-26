#include "function.h"
#include "../../memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParsedValue *create_parsed_function(char *name, DArray parameters,
                                  ParsedValue *body) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type=AST_FUNCTION;
  ParsedFunction *parsedFunction = checked_malloc(sizeof(ParsedFunction));
  parsedValue->data=parsedFunction;
  parsedFunction->name=strcpy(checked_malloc(strlen(name) + 1), name);
  parsedFunction->body = body;
  parsedFunction->parameters=parameters;
  return parsedValue;
}

void free_parameter(void *ptr) {
  char** data = ptr;
  free(*data);
}

void free_function(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedFunction *parsed = parsedValue->data;
  free_parsed(parsed->body);
  free(parsed->body);
  free(parsed->name);
  darray_free(&parsed->parameters, free_parameter);
  free(parsed);
}