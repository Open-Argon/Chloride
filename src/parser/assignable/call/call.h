/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALL_H
#define CALL_H
#include "../../parser.h"

typedef struct {
  ParsedValue *to_call;
  DArray args;          // positional ParsedValue
  DArray *kwargs;       // struct parsed_kwarg, lazy-allocated (NULL if none)
  char *v_arg;          // *ident — identifier to collect extra positional into, or NULL
  char *kw_arg;         // **ident — identifier to collect extra kwargs into, or NULL
  bool must_assign;     // true if v_arg or kw_arg is set
  uint64_t line;
  uint64_t column;
} ParsedCall;

ParsedValueReturn parse_call(char *file, DArray *tokens, size_t *index,
                             ParsedValue *to_call);
void free_parse_call(void *ptr);

#endif // CALL_H