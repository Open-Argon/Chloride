/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "delete.h"
#include "../../err.h"
#include "../../runtime/objects/exceptions/exceptions.h"
#include "../translator.h"

size_t translate_parsed_delete(Translated *translated,
                               ParsedDelete *parsedDelete, ArErr *err) {
  switch (parsedDelete->value->type) {
  case AST_IDENTIFIER: {
    size_t first = 0;
    return first;
  }
  case AST_ACCESS: {
    size_t first = translate_parsed(translated, parsedDelete->value, err);
    return first;
  }
  case AST_ITEM_ACCESS: {
    size_t first = translate_parsed(translated, parsedDelete->value, err);
    return first;
  }
  default:
    fprintf(stderr, "panic: unsupported delete\n");
    exit(EXIT_FAILURE);
  }
}