#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include "arena.h"
#include "diagnostic.h"
#include "str.h"
#include "token.h"

typedef struct TranslationUnit TranslationUnit;

typedef struct ParseResult {
  TranslationUnit* ast;
  ErrorsView errors;
} ParseResult;

/// @brief Scan the source file and generate a list of tokens
Tokens lex(const char* source, Arena* permanent_arena, Arena scratch_arena);

/// @brief Parse tokens into AST
ParseResult parse(const char* src, Tokens tokens, Arena* permanent_arena,
                  Arena scratch_arena);

/// @brief Print Tokens
void print_tokens(const char* src, const Tokens* tokens,
                  const LineNumTable* line_num_table);

#endif // MCC_PARSER_H
