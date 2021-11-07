#ifndef MCC_LEXER_H
#define MCC_LEXER_H

#include "string_view.h"

typedef struct Lexer {
  const char* start;
  const char* current;
  int line;
  int column;
} Lexer;

typedef enum TokenType {
  TOKEN_LEFT_PAREN,  // (
  TOKEN_RIGHT_PAREN, // )
  TOKEN_LEFT_BRACE,  // {
  TOKEN_RIGHT_BRACE, // }
  TOKEN_SEMICOLON,   // ;

  TOKEN_KEYWORD_VOID,
  TOKEN_KEYWORD_INT,
  TOKEN_KEYWORD_RETURN,

  TOKEN_IDENTIFIER,
  TOKEN_INTEGER,

  TOKEN_ERROR,
  TOKEN_EOF
} TokenType;

typedef struct Token {
  StringView src;
  TokenType type;
  int line;
  int column;
} Token;

inline Lexer lexer_create(const char* source)
{
  Lexer lexer;
  lexer.start = source;
  lexer.current = source;
  lexer.line = 1;
  lexer.column = 1;
  return lexer;
}

Token lexer_scan_token(Lexer* lexer);

#endif // MCC_LEXER_H
