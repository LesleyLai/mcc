#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include "ast.h"
#include "utils/str.h"

typedef struct Arena Arena;

TranslationUnit* parse(const char* source, Arena* ast_arena);

#endif
