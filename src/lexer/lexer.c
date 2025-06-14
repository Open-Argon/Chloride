#include "lexer.h"
#include "lex.yy.h"
#include "../string/string.h"

void lexer(LexerState state) {
  size_t line = 1;
  size_t column = 1;
  int ch;
  while ((ch = fgetc(state.file)) != EOF) {
    if (ch == 0 || (ch < 0x20 && ch != '\n' && ch != '\r' && ch != '\t')) {
      fprintf(stderr, "%s:%zu:%zu error: disallowed character\n", state.path,
              line, column);
      exit(1);
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
    Token token_struct = (Token){
      token,
      state.current_line+1,
      state.current_column+1,
      yyget_leng(scanner),
      cloneString(yyget_text(scanner))
    };
    darray_push(state.tokens, &token_struct);
    if (token == TOKEN_NEW_LINE) {
      state.current_line += 1;
      state.current_column = 0;
    } else {
      state.current_column += yyget_leng(scanner);
    }
  }
  yylex_destroy(scanner);
}