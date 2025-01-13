#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include "diagnostic.h"
#include "token.h"

#include <mcc/arena.h>
#include <mcc/str.h>

typedef struct TranslationUnit TranslationUnit;

typedef struct ParseErrorsView {
  size_t length;
  ParseError* data;
} ParseErrorsView;

typedef struct ParseResult {
  TranslationUnit* ast;
  ParseErrorsView errors;
} ParseResult;

/// @brief Scan the source file and generate a list of tokens
Tokens lex(const char* source, Arena* permanent_arena, Arena scratch_arena);

/// @brief Parse tokens into AST
ParseResult parse(const char* src_filename, const char* src, Tokens tokens,
                  Arena* permanent_arena, Arena scratch_arena);

/// @brief A table used to compute line/column numbers
typedef struct LineNumTable {
  uint32_t line_count;
  const uint32_t* line_starts;
} LineNumTable;

typedef struct LineColumn {
  uint32_t line;
  uint32_t column;
} LineColumn;

/**
 * Get the table for calculate line numbers of a file. If the table is already
 * initialized, just return it. Otherwise create the table.
 */
const LineNumTable* get_line_num_table(const char* file_name, StringView src,
                                       Arena* permanent_arena,
                                       Arena scratch_arena);

LineColumn calculate_line_and_column(const LineNumTable* table,
                                     uint32_t offset);

/// @brief Print Tokens
void print_tokens(const char* src, const Tokens* tokens,
                  const LineNumTable* line_num_table);

#endif // MCC_PARSER_H
