#include "lexer.h"
#include "lex.yy.h"

void lexer(LexerState state) {
  yyscan_t scanner;

  yylex_init(&scanner);

  yyset_extra(&state, scanner);

  yyset_in(state.file, scanner);

  int token;
  while ((token = yylex(scanner)) != 0) {
    Token *token_struct =
        create_token(token, state.current_line+1, state.current_column + 1,
                     yyget_text(scanner));
    darray_push(state.tokens, token_struct);
    free(token_struct);
    if (token == TOKEN_NEW_LINE) {
      state.current_line += 1;
      state.current_column = 0;
    } else {
      state.current_column += yyget_leng(scanner);
    }
  }
  yylex_destroy(scanner);
}