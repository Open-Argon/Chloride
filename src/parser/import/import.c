/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "import.h"
#include "../../err.h"
#include "../../lexer/token.h"
#include "../../memory.h"
#include "../parser.h"
#include <string.h>

ParsedValueReturn parse_import(char *file, DArray *tokens, size_t *index) {
  Token *first_token = darray_get(tokens, *index);
  (*index)++;
  ParsedImport import;
  import.as = NULL;
  ArErr err = error_if_finished(file, tokens, index);
  if (err.exists) {
    return (ParsedValueReturn){err, NULL};
  }

  Token *token = darray_get(tokens, *index);
  ParsedValueReturn parsedFile = parse_token(file, tokens, index, true);

  if (parsedFile.err.exists) {
    return parsedFile;
  } else if (!parsedFile.value) {
    return (ParsedValueReturn){create_err(token->line, token->column,
                                          token->length, file, "Syntax Error",
                                          "expected a value"),
                               NULL};
  }

  import.file = parsedFile.value;

  err = error_if_finished(file, tokens, index);
  if (err.exists) {
    free_parsed(parsedFile.value);
    return (ParsedValueReturn){err, NULL};
  }

  token = darray_get(tokens, *index);

  if (*index < tokens->size) {
    Token *token = darray_get(tokens, *index);
    if (token->type == TOKEN_AS) {
      (*index)++;
      err = error_if_finished(file, tokens, index);
      if (err.exists) {
        return (ParsedValueReturn){err, NULL};
      }

      Token *token = darray_get(tokens, *index);

      if (token->type != TOKEN_IDENTIFIER) {
        return (ParsedValueReturn){
            create_err(token->line, token->column, token->length, file,
                       "Syntax Error", "expected identifier"),
            NULL};
      }

      import.as = token->value;
      (*index)++;
    }
  }

  if (import.as) import.as = strcpy(checked_malloc(strlen(import.as) + 1), import.as);
  import.line = first_token->line;
  import.column = first_token->column;
  import.length = first_token->length;

  // todo: add the expose functionality here :)

  ParsedValue *parsedValue = checked_malloc(sizeof(ParsedValue));
  ParsedImport *parsedImport = checked_malloc(sizeof(ParsedImport));
  *parsedImport = import;
  parsedValue->data = parsedImport;
  parsedValue->type = AST_IMPORT;
  return (ParsedValueReturn){no_err, parsedValue};
}

void free_import(void *ptr) {
  ParsedValue *parsedValue = ptr;
  ParsedImport *parsed = parsedValue->data;
  free_parsed(parsed->file);
  free(parsed->file);
  free(parsed->as);
  // todo: add include freeing
  free(parsed);
}