/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "delete.h"
#include "../../hash_data/hash_data.h"
#include "../../parser/assignable/access/access.h"
#include "../../parser/assignable/item/item.h"
#include "../../parser/assignable/identifier/identifier.h"
#include "../translator.h"
#include <stdio.h>
#include <string.h>

size_t translate_parsed_delete(Translated *translated,
                               ParsedDelete *parsedDelete, ArErr *err) {
  switch (parsedDelete->value->type) {
  case AST_IDENTIFIER: {
    ParsedIdentifier *identifier = parsedDelete->value->data;
    size_t length = strlen(identifier->name);
    size_t identifier_pos =
        arena_push(&translated->constants, identifier->name, length);
    set_registers(translated, 1);

    size_t first = push_instruction_byte(translated, OP_SOURCE_LOCATION);
    push_instruction_code(translated, identifier->line);
    push_instruction_code(translated, identifier->column);
    push_instruction_code(translated, length);

    push_instruction_byte(translated, OP_DELETE_IDENTIFIER);
    push_instruction_code(translated, length);
    push_instruction_code(translated, identifier_pos);
    push_instruction_code(translated, siphash64_bytes(identifier->name, length,
                                                      siphash_key_fixed));
    return first;
  }
  case AST_ACCESS: {
    ParsedAccess *parsedAccess = parsedDelete->value->data;
    size_t first = translate_parsed(translated, parsedAccess->to_access, err);
    if (is_error(err))
      return 0;

    push_instruction_byte(translated, OP_SOURCE_LOCATION);
    push_instruction_code(translated, parsedDelete->line);
    push_instruction_code(translated, parsedDelete->column);
    push_instruction_code(translated, parsedDelete->length);

    push_instruction_byte(translated, OP_LOAD_DELATTR_METHOD);

    push_instruction_byte(translated, OP_INIT_CALL);
    push_instruction_code(translated, 1);

    translate_parsed(translated, parsedAccess->access, err);
    if (is_error(err))
      return 0;
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, 0);

    push_instruction_byte(translated, OP_CALL);
    return first;
  }
  case AST_ITEM_ACCESS: {
    ParsedItemAccess *parsedItemAccess = parsedDelete->value->data;
    size_t first = translate_parsed(translated, parsedItemAccess->to_access, err);
    if (is_error(err))
      return 0;

    push_instruction_byte(translated, OP_SOURCE_LOCATION);
    push_instruction_code(translated, parsedDelete->line);
    push_instruction_code(translated, parsedDelete->column);
    push_instruction_code(translated, parsedDelete->length);

    push_instruction_byte(translated, OP_LOAD_DELITEM_METHOD);

    push_instruction_byte(translated, OP_INIT_CALL);
    push_instruction_code(translated, 1);

    
    if (parsedItemAccess->subscripts.size != 1) {
      push_instruction_byte(translated, OP_LOAD_CREATE_TUPLE);
      push_instruction_byte(translated, OP_INIT_CALL);
      push_instruction_code(translated, parsedItemAccess->subscripts.size);
    }
    for (size_t i = 0; i < parsedItemAccess->subscripts.size; i++) {
      DArray *subscript = darray_get(&parsedItemAccess->subscripts, i);
      if (subscript->size != 1) {
        push_instruction_byte(translated, OP_LOAD_SLICE_CLASS);
        push_instruction_byte(translated, OP_INIT_CALL);
        push_instruction_code(translated, subscript->size);
      }
      for (size_t j = 0; j < subscript->size; j++) {
        ParsedValue *item = *(ParsedValue **)darray_get(subscript, j);
        if (item) {
          translate_parsed(translated, item, err);
          if (is_error(err))
            return 0;
        } else {
          push_instruction_byte(translated, OP_LOAD_NULL);
          push_instruction_byte(translated, 0);
        }
        if (subscript->size != 1) {
          push_instruction_byte(translated, OP_INSERT_ARG);
          push_instruction_code(translated, j);
        }
      }
      if (subscript->size != 1) {
        push_instruction_byte(translated, OP_CALL);
      }
      if (parsedItemAccess->subscripts.size != 1) {
        push_instruction_byte(translated, OP_INSERT_ARG);
        push_instruction_code(translated, i);
      }
    }
    if (parsedItemAccess->subscripts.size != 1) {
      push_instruction_byte(translated, OP_CALL);
    }
    push_instruction_byte(translated, OP_INSERT_ARG);
    push_instruction_code(translated, 0);

    push_instruction_byte(translated, OP_CALL);
    return first;
  }
  default:
    fprintf(stderr, "panic: unsupported delete\n");
    exit(EXIT_FAILURE);
  }
}