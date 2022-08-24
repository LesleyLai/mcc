#ifndef MCC_AST_H
#define MCC_AST_H

#include "source_location.h"
#include "utils/str.h"

#include <stdlib.h>

typedef enum ExprType { CONST_EXPR, BINARY_OP_EXPR } ExprType;

typedef enum BinaryOpType {
  BINARY_OP_PLUS,
  BINARY_OP_MINUS,
  BINARY_OP_MULT,
  BINARY_OP_DIVIDE
} BinaryOpType;

typedef struct Expr Expr;

struct ConstExpr {
  int val;
};

struct BinaryOpExpr {
  BinaryOpType binary_op_type;
  Expr* lhs;
  Expr* rhs;
};

typedef struct Expr {
  SourceRange source_range;
  ExprType type;
  union {
    struct ConstExpr const_expr;
    struct BinaryOpExpr binary_op;
  };
} Expr;

typedef enum StatementType { COMPOUND_STMT, RETURN_STMT } StatementType;

typedef struct Stmt Stmt;

typedef struct CompoundStmt {
  size_t statement_count;
  Stmt* statements;
} CompoundStmt;

typedef struct ReturnStmt {
  Expr* expr;
} ReturnStmt;

typedef struct Stmt {
  SourceRange source_range;
  StatementType type;
  union {
    CompoundStmt compound;
    ReturnStmt ret;
  };
} Stmt;

typedef struct FunctionDecl {
  SourceRange source_range;
  StringView name;
  CompoundStmt* body;
} FunctionDecl;

#endif // MCC_AST_H
