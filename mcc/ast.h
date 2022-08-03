#ifndef MCC_AST_H
#define MCC_AST_H

#include "string_view.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct SourceLocation {
  uint32_t line;
  uint32_t column;
} SourceLocation;

typedef struct SourceRange {
  SourceLocation first;
  SourceLocation last;
} SourceRange;

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
