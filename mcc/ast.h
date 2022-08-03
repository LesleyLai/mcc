#ifndef MCC_AST_H
#define MCC_AST_H

#include "source_location.h"
#include "string_view.h"

#include <stdlib.h>

typedef struct Expr {
  SourceRange source_range;
  int val;
} Expr;

typedef enum StatementType { COMPOUND_STMT, RETURN_STMT } StatementType;

typedef struct Stmt {
  SourceRange source_range;
  StatementType type;
} Stmt;

typedef struct CompoundStmt {
  Stmt base;
  size_t statement_count;
  Stmt** statements;
} CompoundStmt;

typedef struct ReturnStmt {
  Stmt base;
  Expr* expr;
} ReturnStmt;

typedef struct FunctionDecl {
  SourceRange source_range;
  StringView name;
  CompoundStmt* body;
} FunctionDecl;

#endif // MCC_AST_H
