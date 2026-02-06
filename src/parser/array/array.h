/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ARRAY_H
#define ARRAY_H
#include "../parser.h"

ParsedValueReturn parse_array(char *file, DArray *tokens, size_t *index);

void free_parsed_array(void *ptr);

#endif // ARRAY_H