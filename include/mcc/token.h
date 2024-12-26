#ifndef MCC_TOKEN_H
#define MCC_TOKEN_H

#include "source_location.h"
#include "str.h"

typedef enum TokenType {
  TOKEN_INVALID = 0,

  TOKEN_LEFT_PAREN,    // (
  TOKEN_RIGHT_PAREN,   // )
  TOKEN_LEFT_BRACE,    // {
  TOKEN_RIGHT_BRACE,   // }
  TOKEN_LEFT_BRACKET,  // [
  TOKEN_RIGHT_BRACKET, // ]

  TOKEN_PLUS,                  // +
  TOKEN_PLUS_PLUS,             // ++
  TOKEN_PLUS_EQUAL,            // +=
  TOKEN_MINUS,                 // -
  TOKEN_MINUS_MINUS,           // --
  TOKEN_MINUS_EQUAL,           // -=
  TOKEN_MINUS_GREATER,         // ->
  TOKEN_STAR,                  // *
  TOKEN_STAR_EQUAL,            // *=
  TOKEN_SLASH,                 // /
  TOKEN_SLASH_EQUAL,           // /=
  TOKEN_PERCENT,               // %
  TOKEN_PERCENT_EQUAL,         // %=
  TOKEN_TILDE,                 // ~
  TOKEN_AMPERSAND,             // &
  TOKEN_AMPERSAND_AMPERSAND,   // &&
  TOKEN_AMPERSAND_EQUAL,       // &=
  TOKEN_BAR,                   // |
  TOKEN_BAR_BAR,               // ||
  TOKEN_BAR_EQUAL,             // |=
  TOKEN_CARET,                 // ^
  TOKEN_CARET_EQUAL,           // ^=
  TOKEN_EQUAL,                 // =
  TOKEN_EQUAL_EQUAL,           // ==
  TOKEN_NOT,                   // !
  TOKEN_NOT_EQUAL,             // !=
  TOKEN_LESS,                  // <
  TOKEN_LESS_LESS,             // <<
  TOKEN_LESS_LESS_EQUAL,       // <<=
  TOKEN_LESS_EQUAL,            // <=
  TOKEN_GREATER,               // >
  TOKEN_GREATER_GREATER,       // >>
  TOKEN_GREATER_GREATER_EQUAL, // >>=
  TOKEN_GREATER_EQUAL,         // >=

  TOKEN_COMMA,     // ,
  TOKEN_DOT,       // .
  TOKEN_QUESTION,  // ?
  TOKEN_SEMICOLON, // ;
  TOKEN_COLON,     // :

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
