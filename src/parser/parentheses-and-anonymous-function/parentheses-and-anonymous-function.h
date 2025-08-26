/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef parentheses_and_anonymous_function_H
#define parentheses_and_anonymous_function_H
#include "../parser.h"
#include "../../lexer/token.h"

ParsedValueReturn parse_parentheses(char *file, DArray *tokens, size_t *index);

#endif // parentheses_and_anonymous_function_H