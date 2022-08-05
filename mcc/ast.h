#ifndef MCC_AST_H
#define MCC_AST_H

#include "source_location.h"
#include "utils/str.h"

#include <stdlib.h>

typedef enum ExprType { CONST_EXPR, BINARY_OP_EXPR } ExprType;

typedef struct Expr {
  SourceRange source_range;
  ExprType type;
} Expr;

typedef struct ConstExpr {
  Expr base;
  int val;
} ConstExpr;

typedef enum BinaryOpType {
  BINARY_OP_PLUS,
  BINARY_OP_MINUS,
  BINARY_OP_MULT,
  BINARY_OP_DIVIDE
} BinaryOpType;

typedef struct BinaryOpExpr {
  Expr base;
  BinaryOpType binary_op_type;
  Expr* lhs;
  Expr* rhs;
} BinaryOp;

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
