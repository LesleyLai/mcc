#ifndef MCC_TOKEN_H
#define MCC_TOKEN_H

#include "source_location.h"
#include "utils/str.h"

typedef enum TokenType {
  TOKEN_LEFT_PAREN,  // (
  TOKEN_RIGHT_PAREN, // )
  TOKEN_LEFT_BRACE,  // {
  TOKEN_RIGHT_BRACE, // }
  TOKEN_SEMICOLON,   // ;

  TOKEN_PLUS,    // +
  TOKEN_MINUS,   // -
  TOKEN_STAR,    // *
  TOKEN_SLASH,   // /
  TOKEN_PERCENT, // %

  TOKEN_TILDE, // ~

  TOKEN_KEYWORD_VOID,
  TOKEN_KEYWORD_INT,
  TOKEN_KEYWORD_RETURN,

  TOKEN_IDENTIFIER,
  TOKEN_INTEGER,

  TOKEN_ERROR,
  TOKEN_EOF,

  TOKEN_TYPES_COUNT,
} TokenType;

typedef struct Token {
  StringView src;
  TokenType type;
  SourceLocation location;
} Token;

/// @brief A view of tokens
typedef struct Tokens {
  Token* begin; // Points to the first element of the token array
  Token* end;   // Points to one plus the last element of the token array
} Tokens;

/// @brief Print Tokens
void print_tokens(Tokens* tokens);

#endif // MCC_TOKEN_H
