/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FUNCTION_H
#define FUNCTION_H
#include "../../lexer/token.h" // for Token
#include "../parser.h"

typedef struct {
  char * name;
  DArray parameters;
  ParsedValue *body;
} ParsedFunction;

ParsedValue *create_parsed_function(char *name, DArray parameters,
                                  ParsedValue *body);

void free_function(void *ptr);

void free_parameter(void *ptr);

#endif // FUNCTION_H