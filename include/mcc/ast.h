#ifndef MCC_AST_H
#define MCC_AST_H

#include "hash_table.h"
#include "source_location.h"
#include "str.h"
#include "type.h"

#include <stdlib.h>

typedef enum ExprType {
  EXPR_INVALID = 0,
  EXPR_CONST,
  EXPR_VARIABLE,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_TERNARY,
  EXPR_CALL,
} ExprTag;

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

  BINARY_OP_ASSIGNMENT, // a = b
  BINARY_OP_PLUS_EQUAL,
  BINARY_OP_MINUS_EQUAL,
  BINARY_OP_MULT_EQUAL,
  BINARY_OP_DIVIDE_EQUAL,
  BINARY_OP_MOD_EQUAL,
  BINARY_OP_BITWISE_AND_EQUAL,
  BINARY_OP_BITWISE_OR_EQUAL,
  BINARY_OP_BITWISE_XOR_EQUAL,
  BINARY_OP_SHIFT_LEFT_EQUAL,
  BINARY_OP_SHIFT_RIGHT_EQUAL,
} BinaryOpType;

typedef struct Expr Expr;
typedef struct IdentifierInfo IdentifierInfo; // identifier
typedef struct Scope Scope;

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

struct TernaryExpr {
  Expr* cond;
  Expr* true_expr;
  Expr* false_expr;
};

struct CallExpr {
  Expr* function;
  Expr** args;
  uint32_t arg_count;
};

typedef struct Expr {
  SourceRange source_range;
  ExprTag tag;
  const Type* type; // C type (e.g. void, int, int*)
  union {
    struct ConstExpr const_expr;
    struct UnaryOpExpr unary_op;
    struct BinaryOpExpr binary_op;
    IdentifierInfo* variable;
    struct TernaryExpr ternary;
    struct CallExpr call;
  };
} Expr;

typedef enum StorageClass {
  STORAGE_CLASS_NONE = 0,
  STORAGE_CLASS_EXTERN,
  STORAGE_CLASS_STATIC,
} StorageClass;

typedef struct VariableDecl {
  const Type* type;
  StorageClass storage_class;
  SourceRange source_range;
  IdentifierInfo* identifier;
  Expr* initializer; // An optional initializer
} VariableDecl;

typedef enum StmtTag {
  STMT_INVALID = 0,
  STMT_EMPTY,
  STMT_EXPR, // An expression statement
  STMT_COMPOUND,
  STMT_RETURN,
  STMT_IF,
  STMT_WHILE,
  STMT_DO_WHILE,
  STMT_FOR,
  STMT_BREAK,
  STMT_CONTINUE,
} StmtTag;

typedef struct BlockItem BlockItem;

typedef struct Block {
  size_t child_count;
  BlockItem* children;
} Block;

typedef struct Stmt Stmt;

enum ForInitTag { FOR_INIT_INVALID, FOR_INIT_DECL, FOR_INIT_EXPR };

typedef struct ForInit {
  enum ForInitTag tag;
  union {
    VariableDecl* decl;
    Expr* expr; // optional, can be nullptr
  };
} ForInit;

struct Stmt {
  SourceRange source_range;
  StmtTag tag;
  union {
    Block compound;
    struct ReturnStmt {
      Expr* expr;
    } ret;
    Expr* expr;
    struct IfStmt {
      Expr* cond;
      Stmt* then;
      Stmt* els; // optional, can be nullptr
    } if_then;

    // while or do while loop
    struct While {
      Expr* cond;
      Stmt* body;
    } while_loop;

    struct For {
      ForInit init;
      Expr* cond; // optional, can be nullptr
      Expr* post; // optional, can be nullptr
      Stmt* body;
    } for_loop;
  };
};

typedef struct Parameters {
  uint32_t length;
  IdentifierInfo** data;
} Parameters;

typedef struct FunctionDecl {
  const Type* return_type;
  StorageClass storage_class;
  SourceRange source_range;
  IdentifierInfo* name;
  Parameters params;
  Block* body; // Optional
} FunctionDecl;

typedef enum DeclTag {
  DECL_INVALID = 0,
  DECL_VAR,
  DECL_FUNC,
} DeclTag;

typedef struct Decl {
  DeclTag tag;
  union {
    struct VariableDecl var;
    struct FunctionDecl* func;
  };
} Decl;

enum BlockItemTag {
  BLOCK_ITEM_STMT,
  BLOCK_ITEM_DECL,
};

typedef struct BlockItem {
  enum BlockItemTag tag;
  union {
    Stmt stmt;
    Decl decl;
  };
} BlockItem;

typedef struct TranslationUnit {
  uint32_t decl_count;
  Decl* decls;

  struct SymbolTable* symbol_table;
} TranslationUnit;

StringView string_from_ast(const TranslationUnit* tu, Arena* permanent_arena);

#endif // MCC_AST_H
