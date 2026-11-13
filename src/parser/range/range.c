/*
 * SPDX-FileCopyrightText: 2025, 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "range.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_parse_range(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedRange *parsedRange = parsedValue->data;

  free_parsed(parsedRange->start);
  free(parsedRange->start);
  free_parsed(parsedRange->stop);
  free(parsedRange->stop);
  free(parsedRange);
}