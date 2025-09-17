/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "assignment.h"
#include "../../hash_data/hash_data.h"
#include "../../parser/assignable/identifier/identifier.h"
#include "../../parser/assignable/access/access.h"
#include "../translator.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t translate_parsed_assignment(Translated *translated,
                                   ParsedAssign *assignment, ArErr *err) {
  set_registers(translated, 1);
  DArray *old_return_jumps = translated->return_jumps;
  translated->return_jumps = NULL;
  size_t first = translate_parsed(translated, assignment->from, err);
  if (err->exists)
    return first;
  switch (assignment->to->type) {
  case AST_IDENTIFIER:;
    ParsedIdentifier *identifier = assignment->to->data;
    size_t length = strlen(identifier->name);
    size_t offset =
        arena_push(&translated->constants, identifier->name, length);
        
    push_instruction_byte(translated, OP_SOURCE_LOCATION);
    push_instruction_code(translated, identifier->line);
    push_instruction_code(translated, identifier->column);
    push_instruction_code(translated, length);

    push_instruction_byte(translated, OP_ASSIGN);
    push_instruction_code(translated, length);
    push_instruction_code(translated, offset);
    push_instruction_code(translated,
                          siphash64_bytes(identifier->name, length,
                                          siphash_key_fixed_for_translator));
    push_instruction_byte(translated, 0);
    break;
  case AST_ACCESS:;
    ParsedAccess *access = assignment->to->data;
    break;
  default:
    fprintf(stderr, "panic: unsupported assignment\n");
    exit(EXIT_FAILURE);
  }
  translated->return_jumps = old_return_jumps;
  return first;
}