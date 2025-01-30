#ifndef MCC_TOKEN_H
#define MCC_TOKEN_H

#include "source_location.h"
#include "str.h"

typedef enum TokenTag : char {
  TOKEN_INVALID = 0,

  TOKEN_LEFT_PAREN,    // (
  TOKEN_RIGHT_PAREN,   // )
  TOKEN_LEFT_BRACE,    // {
  TOKEN_RIGHT_BRACE,   // }
  TOKEN_LEFT_BRACKET,  // [
  TOKEN_RIGHT_BRACKET, // ]

  TOKEN_PLUS,                // +
  TOKEN_PLUS_PLUS,           // ++
  TOKEN_PLUS_EQUAL,          // +=
  TOKEN_MINUS,               // -
  TOKEN_MINUS_MINUS,         // --
  TOKEN_MINUS_EQUAL,         // -=
  TOKEN_MINUS_GREATER,       // ->
  TOKEN_STAR,                // *
  TOKEN_STAR_EQUAL,          // *=
  TOKEN_SLASH,               // /
  TOKEN_SLASH_EQUAL,         // /=
  TOKEN_PERCENT,             // %
  TOKEN_PERCENT_EQUAL,       // %=
  TOKEN_TILDE,               // ~
  TOKEN_AMPERSAND,           // &
  TOKEN_AMPERSAND_AMPERSAND, // &&
  TOKEN_AMPERSAND_EQUAL,     // &=
  TOKEN_BAR,                 // |
  TOKEN_BAR_BAR,             // ||
  TOKEN_BAR_EQUAL,           // |=
  TOKEN_CARET,               // ^
  TOKEN_CARET_EQUAL,         // ^=
  TOKEN_EQUAL,               // =

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
  TOKEN_KEYWORD_TYPEDEF,
  TOKEN_KEYWORD_IF,
  TOKEN_KEYWORD_ELSE,
  TOKEN_KEYWORD_WHILE,
  TOKEN_KEYWORD_DO,
  TOKEN_KEYWORD_FOR,
  TOKEN_KEYWORD_BREAK,
  TOKEN_KEYWORD_CONTINUE,

  TOKEN_IDENTIFIER,
  TOKEN_INTEGER,

  TOKEN_ERROR,
  TOKEN_EOF,

  TOKEN_TYPES_COUNT,
} TokenTag;

typedef struct Token {
  TokenTag tag;
  uint32_t start; // The offset of the starting character in a token
  uint32_t size;
} Token;

/// @brief An SOA view of tokens
typedef struct Tokens {
  TokenTag* token_types;
  uint32_t* token_starts;
  uint32_t* token_sizes;
  uint32_t token_count;
} Tokens;

inline static Token get_token(const Tokens* tokens, uint32_t i)
{
  MCC_ASSERT(i < tokens->token_count);
  return MCC_COMPOUND_LITERAL(Token){.tag = tokens->token_types[i],
                                     .start = tokens->token_starts[i],
                                     .size = tokens->token_sizes[i]};
}

#endif // MCC_TOKEN_H
