#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
  TOKEN_STRING = 256,
  TOKEN_NUMBER,
  TOKEN_FRACTION,
  TOKEN_IDENTIFIER,
  TOKEN_KEYWORD,
  TOKEN_NEW_LINE,
  TOKEN_INDENT,

  TOKEN_ASSIGN,
  TOKEN_ASSIGN_PLUS,
  TOKEN_ASSIGN_MINUS,
  TOKEN_ASSIGN_FLOORDIV,
  TOKEN_ASSIGN_SLASH,
  TOKEN_ASSIGN_MODULO,
  TOKEN_ASSIGN_STAR,
  TOKEN_ASSIGN_CARET,

  // Operators
  TOKEN_AND,      // &&
  TOKEN_OR,       // ||
  TOKEN_NOT_IN,   // not in
  TOKEN_LE,       // <=
  TOKEN_GE,       // >=
  TOKEN_LT,       // <
  TOKEN_GT,       // >
  TOKEN_NE,       // !=
  TOKEN_EQ,       // ==
  TOKEN_PLUS,     // +
  TOKEN_MINUS,    // -
  TOKEN_MODULO,   // %
  TOKEN_STAR,     // *
  TOKEN_FLOORDIV, // //
  TOKEN_SLASH,    // /
  TOKEN_CARET,    // ^

  // Keywords
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_FOREVER,
  TOKEN_FOR,
  TOKEN_BREAK,
  TOKEN_CONTINUE,
  TOKEN_RETURN,
  TOKEN_LET,
  TOKEN_IMPORT,
  TOKEN_FROM,
  TOKEN_DO,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_NULL,
  TOKEN_DELETE,
  TOKEN_NOT,
  TOKEN_IN,
  TOKEN_TRY,
  TOKEN_CATCH,

  // parentheses, brackets, and braces
  TOKEN_LPAREN,   // (
  TOKEN_RPAREN,   // )
  TOKEN_LBRACKET, // [
  TOKEN_RBRACKET, // ]
  TOKEN_LBRACE,   // {
  TOKEN_RBRACE,   // }

  TOKEN_DOT,
  TOKEN_COMMA,
  TOKEN_COLON,
} TokenType;

typedef struct {
  TokenType type;
  int line;
  int column;
  char *value;
} Token;

Token *create_token(TokenType type, int line, int column, char *value);
void free_token(void *ptr);
#endif