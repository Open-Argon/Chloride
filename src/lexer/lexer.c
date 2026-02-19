/*
 * SPDX-FileCopyrightText: 2025 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lexer.h"
#include "../err.h"
#include "../string/string.h"
#include "lex.yy.h"
#include "token.h"

ArErr lexer(LexerState state) {

  yyscan_t yyscanner;

  yylex_init(&yyscanner);

  yyset_extra(&state, yyscanner);

  yy_scan_string(state.content, yyscanner);

  int token;
  while ((token = yylex(yyscanner)) != 0) {
    if (token == TOKEN_INVALID) {
      ArErr err = create_err(state.current_line + 1, state.current_column + 1,
                             yyget_leng(yyscanner), state.path, "Syntax Error",
                             "Invalid Token '%s'", yyget_text(yyscanner));
      yylex_destroy(yyscanner);
      return err;
    }
    Token token_struct =
        (Token){token, state.current_line + 1, state.current_column + 1,
                yyget_leng(yyscanner), cloneString(yyget_text(yyscanner))};
    darray_push(state.tokens, &token_struct);
    if (token == TOKEN_NEW_LINE) {
      state.current_line += 1;
      state.current_column = 0;
    } else {
      state.current_column += yyget_leng(yyscanner);
    }
  }
  yylex_destroy(yyscanner);
  return no_err;
}