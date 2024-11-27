#ifndef MCC_LEXER_H
#define MCC_LEXER_H

#include <stdint.h>

#include "token.h"

// The lexer consumes source code and produces tokens lazily

typedef struct Lexer {
  const char* start;
  const char* previous;
  const char* current;
  uint32_t line;
  uint32_t column;
} Lexer;

Lexer lexer_create(const char* source);
Token lexer_scan_token(Lexer* lexer);

// @brief Scan the source file and generate a list of tokens
Tokens lex(const char* source, Arena* permanent_arena, Arena scratch_arena);

#endif // MCC_LEXER_H
