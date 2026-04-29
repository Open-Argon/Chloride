/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FUNCTION_H
#define FUNCTION_H
#include "../parser.h"

struct default_value_parameter {
  char *name;
  ParsedValue *value;
};

typedef struct {
  char *name;
  DArray parameters;
  DArray *default_value_parameters;
  char *v_parameter;
  char *kw_parameter;
  ParsedValue *body;
} ParsedFunction;

ParsedValue *create_parsed_function(char *name, DArray parameters,
                                    DArray *default_value_parameters,
                                    char *v_parameter,
                                    char *kw_parameter, ParsedValue *body);

void free_function(void *ptr);

void free_parameter(void *ptr);

void free_default_value_parameter(void *ptr);

#endif // FUNCTION_H