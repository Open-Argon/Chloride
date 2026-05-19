/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CONDITIONAL_EXPRESSION_H
#define CONDITIONAL_EXPRESSION_H
#include "../parser.h"

typedef struct {
  ParsedValue *condition;
  ParsedValue *true_body;
  ParsedValue *false_body;
} ParsedConditionalExpression;

ParsedValueReturn parse_conditional_expression(char *file, DArray *tokens,
                                               size_t *index,
                                               ParsedValue *condition);

void free_expression_conditional(void *ptr);

#endif // CONDITIONAL_EXPRESSION_H