#ifndef MCC_PARSER_H
#define MCC_PARSER_H

#include "ast.h"
#include "string_view.h"

typedef struct Arena Arena;

FunctionDecl* parse(const char* source, Arena* ast_arena);

#endif