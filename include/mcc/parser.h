#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include "diagnostic.h"
#include "token.h"

#include <mcc/arena.h>
#include <mcc/str.h>

typedef struct TranslationUnit TranslationUnit;

typedef struct ParseErrorsView {
  size_t size;
  ParseError* data;
} ParseErrorsView;

typedef struct ParseResult {
  TranslationUnit* ast;
  ParseErrorsView errors;
} ParseResult;

/// @brief Scan the source file and generate a list of tokens
Tokens lex(const char* source, Arena* permanent_arena, Arena scratch_arena);

/// @brief Parse tokens into AST
ParseResult parse(Tokens tokens, Arena* permanent_arena, Arena scratch_arena);

#endif // MCC_PARSER_H
