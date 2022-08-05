#ifndef MCC_LEXER_H
#define MCC_LEXER_H

#include <stdint.h>

#include "source_location.h"
#include "utils/str.h"

// The lexer consumes source code and produces tokens lazily

typedef struct Lexer {
  const char* start;
  const char* current;
  uint32_t line;
  uint32_t column;
} Lexer;

typedef enum TokenType {
  TOKEN_LEFT_PAREN,  // (
  TOKEN_RIGHT_PAREN, // )
  TOKEN_LEFT_BRACE,  // {
  TOKEN_RIGHT_BRACE, // }
  TOKEN_SEMICOLON,   // ;

  TOKEN_PLUS,  // +
  TOKEN_MINUS, // -
  TOKEN_STAR,  // *
  TOKEN_SLASH, // /

  TOKEN_KEYWORD_VOID,
  TOKEN_KEYWORD_INT,
  TOKEN_KEYWORD_RETURN,

  TOKEN_IDENTIFIER,
  TOKEN_INTEGER,

  TOKEN_ERROR,
  TOKEN_EOF,

  TOKEN_TYPES_COUNT
} TokenType;

typedef struct Token {
  StringView src;
  TokenType type;
  SourceLocation location;
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
