#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include "ast.h"
#include "diagnostic.h"
#include "token.h"

#include "utils/arena.h"
#include "utils/str.h"

typedef struct ParseErrorsView {
  size_t size;
  ParseError* data;
} ParseErrorsView;

typedef struct ParseResult {
  TranslationUnit* ast;
  ParseErrorsView errors;
} ParseResult;

ParseResult parse(Tokens tokens, Arena* permanent_arena, Arena scratch_arena);

#endif
