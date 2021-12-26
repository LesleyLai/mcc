#ifndef MCC_AST_H
#define MCC_AST_H

#include "string_view.h"
#include <stdlib.h>

typedef struct Expr {
  int val;
} Expr;

typedef enum StatementType { COMPOUND_STMT, RETURN_STMT } StatementType;

typedef struct Stmt Stmt;

typedef struct CompoundStmt {
  size_t statement_count;
  Stmt** statements;
} CompoundStmt;

typedef struct ReturnStmt {
  Expr* expr;
} ReturnStmt;

typedef struct Stmt {
  StatementType type;
  union {
    CompoundStmt compound_statement;
    ReturnStmt return_statement;
  };
} Stmt;

typedef struct FunctionDecl {
  StringView name;
  CompoundStmt* body;
} FunctionDecl;

#endif // MCC_AST_H
