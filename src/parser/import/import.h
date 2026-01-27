/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IMPORT_PARSE_H
#define IMPORT_PARSE_H
#include "../parser.h"

typedef struct {
  ParsedValue *file;
  char *as;
  bool expose_all;
  DArray expose;
  size_t line;
  size_t column;
  size_t length;
} ParsedImport;

typedef struct {
  char *identifier;
  char *as;
  size_t line;
  size_t column;
  size_t length;
} ParsedImportExpose;

ParsedValueReturn parse_import(char *file, DArray *tokens, size_t *index);

void free_import(void *ptr);

#endif // IMPORT_PARSE_H