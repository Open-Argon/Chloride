/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "function.h"
#include "../../memory.h"
#include <stdlib.h>
#include <string.h>

ParsedValue *create_parsed_function(char *name, DArray parameters,
                                    DArray *default_value_parameters,
                                    char *v_parameter,
                                    char *kw_parameter, ParsedValue *body) {
  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  parsedValue->type = AST_FUNCTION;
  ParsedFunction *parsedFunction = checked_malloc(sizeof(ParsedFunction));
  parsedValue->data = parsedFunction;
  parsedFunction->name = strcpy(checked_malloc(strlen(name) + 1), name);
  parsedFunction->body = body;
  parsedFunction->parameters = parameters;
  parsedFunction->v_parameter = v_parameter;
  parsedFunction->kw_parameter = kw_parameter;
  parsedFunction->default_value_parameters = default_value_parameters;
  return parsedValue;
}

void free_default_value_parameter(void *ptr) {
  struct default_value_parameter *data = ptr;
  free_parsed(data->value);
  free(data->value);
  free(data->name);
}

void free_parameter(void *ptr) {
  char **data = ptr;
  free(*data);
}

void free_function(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedFunction *parsed = parsedValue->data;
  free_parsed(parsed->body);
  free(parsed->body);
  free(parsed->name);
  darray_free(&parsed->parameters, free_parameter);
  if (parsed->default_value_parameters){
    darray_free(parsed->default_value_parameters, free_default_value_parameter);
    free(parsed->default_value_parameters);
  }
  if (parsed->v_parameter)
    free(parsed->v_parameter);
  if (parsed->kw_parameter)
    free(parsed->kw_parameter);
  free(parsed);
}