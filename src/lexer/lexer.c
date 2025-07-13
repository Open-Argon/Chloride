/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lexer.h"
#include "../string/string.h"
#include "lex.yy.h"

ArErr lexer(LexerState state) {
  size_t line = 1;
  size_t column = 1;
  int ch;
  while ((ch = fgetc(state.file)) != EOF) {
    if (ch == 0 || (ch < 0x20 && ch != '\n' && ch != '\r' && ch != '\t')) {
      return create_err(line, column, 1, state.path, "Syntax Error", "disallowed character");
    }

    if (ch == '\n') {
      line++;
      column = 1;
    } else {
      column++;
    }
  }
  rewind(state.file);

  yyscan_t scanner;

  yylex_init(&scanner);

  yyset_extra(&state, scanner);

  yyset_in(state.file, scanner);

  int token;
  while ((token = yylex(scanner)) != 0) {
    if (token == TOKEN_INVALID) {
      ArErr err = create_err(state.current_line + 1, state.current_column + 1,
                        yyget_leng(scanner), state.path, "Syntax Error",
                        "Invalid Token '%s'", yyget_text(scanner));
      yylex_destroy(scanner);
      return err;
    }
    Token token_struct =
        (Token){token, state.current_line + 1, state.current_column + 1,
                yyget_leng(scanner), cloneString(yyget_text(scanner))};
    darray_push(state.tokens, &token_struct);
    if (token == TOKEN_NEW_LINE) {
      state.current_line += 1;
      state.current_column = 0;
    } else {
      state.current_column += yyget_leng(scanner);
    }
  }
  yylex_destroy(scanner);
  return no_err;
}