/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LIST_H
#define LIST_H
#include "../../lexer/token.h" // for Token
#include "../parser.h"

ParsedValueReturn parse_list(char *file, DArray *tokens, size_t *index);

void free_parsed_list(void *ptr);

#endif // LIST_H