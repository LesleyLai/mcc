#ifndef MCC_AST_H
#define MCC_AST_H

#include "source_location.h"
#include "str.h"

#include <stdlib.h>

typedef enum ExprType {
  EXPR_INVALID = 0,
  EXPR_CONST,
  EXPR_UNARY,
  EXPR_BINARY
} ExprType;

typedef enum UnaryOpType {
  UNARY_OP_INVALID = 0,
  UNARY_OP_NEGATION,                // -
  UNARY_OP_BITWISE_TYPE_COMPLEMENT, // ~
  UNARY_OP_NOT,                     // !
} UnaryOpType;

typedef enum BinaryOpType {
  BINARY_OP_INVALID = 0,

  BINARY_OP_PLUS,
  BINARY_OP_MINUS,
  BINARY_OP_MULT,
  BINARY_OP_DIVIDE,
  BINARY_OP_MOD,

  BINARY_OP_BITWISE_AND,
  BINARY_OP_BITWISE_OR,
  BINARY_OP_BITWISE_XOR,
  BINARY_OP_SHIFT_LEFT,
  BINARY_OP_SHIFT_RIGHT,

  BINARY_OP_AND, // logical and
  BINARY_OP_OR,  // logical or

  BINARY_OP_EQUAL,
  BINARY_OP_NOT_EQUAL,
  BINARY_OP_LESS,
  BINARY_OP_LESS_EQUAL,
  BINARY_OP_GREATER,
  BINARY_OP_GREATER_EQUAL,
} BinaryOpType;

typedef struct Expr Expr;

struct ConstExpr {
  int32_t val;
};

struct UnaryOpExpr {
  UnaryOpType unary_op_type;
  Expr* inner_expr;
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
    struct UnaryOpExpr unary_op;
    struct BinaryOpExpr binary_op;
  };
} Expr;

typedef enum StatementType {
  STMT_INVALID = 0,
  STMT_COMPOUND,
  STMT_RETURN
} StatementType;

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

typedef struct TranslationUnit {
  size_t decl_count;
  FunctionDecl* decls;
} TranslationUnit;

void ast_print_translation_unit(TranslationUnit* tu);

#endif // MCC_AST_H
