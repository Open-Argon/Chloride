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
#include <stddef.h>

size_t count_newlines(const char *str, size_t len) {
  size_t count = 0;
  for (size_t i = 0; i < len; i++) {
    if (str[i] == '\n') {
      count++;
    }
  }
  return count;
}

size_t chars_after_last_newline(const char *str, size_t len) {
  ssize_t last_newline = -1;
  for (size_t i = 0; i < len; i++) {
    if (str[i] == '\n') {
      last_newline = i;
    }
  }
  return len - (last_newline + 1);
}

ArErr lexer(LexerState state) {

  yyscan_t yyscanner;

  yylex_init(&yyscanner);

  yyset_extra(&state, yyscanner);

  yy_scan_string(state.content, yyscanner);

  int token;
  while ((token = yylex(yyscanner)) != 0) {
    if (token == TOKEN_INVALID) {
      ArErr err = path_specific_create_err(
          state.current_line + 1, state.current_column + 1,
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
      size_t newlines =
          count_newlines(yyget_text(yyscanner), yyget_leng(yyscanner));
      if (newlines) {
        state.current_line +=newlines;
        state.current_column = chars_after_last_newline(yyget_text(yyscanner), yyget_leng(yyscanner));
      } else {
        state.current_column += yyget_leng(yyscanner);
      }
    }
  }
  yylex_destroy(yyscanner);
  return no_err;
}