#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include "ast.h"
#include "diagnostic.h"

#include "utils/str.h"

typedef struct Arena Arena;

// TODO: use vector
typedef struct ParseErrorsView {
  size_t size;
  ParseError* data;
} ParseErrorsView;

typedef struct ParseResult {
  TranslationUnit* ast;
  ParseErrorsView errors;
} ParseResult;

ParseResult parse(const char* source, Arena* ast_arena);

#endif
