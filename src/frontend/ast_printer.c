#include <mcc/ast.h>
#include <mcc/prelude.h>

#include "symbol_table.h"

static void print_str(StringView str)
{
  printf("%.*s", (int)str.size, str.start);
}

static void print_source_range(SourceRange range)
{
  // TODO: print line and columns
  printf("<%d..%d>", range.begin, range.end);
}

static const char* unary_op_name(UnaryOpType unary_op_type)
{
  switch (unary_op_type) {
  case UNARY_OP_INVALID: MCC_UNREACHABLE();
  case UNARY_OP_NEGATION: return "negation";
  case UNARY_OP_BITWISE_TYPE_COMPLEMENT: return "complement";
  case UNARY_OP_NOT: return "not";
  }
  MCC_ASSERT_MSG(false, "invalid enum");
}

static const char* binary_op_name(BinaryOpType binary_op_type)
{
  switch (binary_op_type) {
  case BINARY_OP_INVALID: MCC_UNREACHABLE();
  case BINARY_OP_PLUS: return "+";
  case BINARY_OP_MINUS: return "-";
  case BINARY_OP_MULT: return "*";
  case BINARY_OP_DIVIDE: return "/";
  case BINARY_OP_MOD: return "mod";
  case BINARY_OP_BITWISE_AND: return "bit-and";
  case BINARY_OP_BITWISE_OR: return "bit-or";
  case BINARY_OP_BITWISE_XOR: return "xor";
  case BINARY_OP_SHIFT_LEFT: return "<<";
  case BINARY_OP_SHIFT_RIGHT: return ">>";
  case BINARY_OP_AND: return "and";
  case BINARY_OP_OR: return "or";
  case BINARY_OP_EQUAL: return "equal";
  case BINARY_OP_NOT_EQUAL: return "!=";
  case BINARY_OP_LESS: return "<";
  case BINARY_OP_LESS_EQUAL: return "<=";
  case BINARY_OP_GREATER: return ">";
  case BINARY_OP_GREATER_EQUAL: return ">=";
  case BINARY_OP_ASSIGNMENT: return "=";
  case BINARY_OP_PLUS_EQUAL: return "+=";
  case BINARY_OP_MINUS_EQUAL: return "-=";
  case BINARY_OP_MULT_EQUAL: return "*=";
  case BINARY_OP_DIVIDE_EQUAL: return "/=";
  case BINARY_OP_MOD_EQUAL: return "%=";
  case BINARY_OP_BITWISE_AND_EQUAL: return "bit-and=";
  case BINARY_OP_BITWISE_OR_EQUAL: return "bit-or=";
  case BINARY_OP_BITWISE_XOR_EQUAL: return "xor=";
  case BINARY_OP_SHIFT_LEFT_EQUAL: return "<<=";
  case BINARY_OP_SHIFT_RIGHT_EQUAL: return ">>=";
  }
  MCC_ASSERT_MSG(false, "invalid enum");
}

static void ast_print_expr(const Expr* expr, int indent)
{
  switch (expr->tag) {
  case EXPR_INVALID: MCC_UNREACHABLE();
  case EXPR_CONST:
    printf("%*sIntegerLiteral ", indent, "");
    print_source_range(expr->source_range);
    printf(" %i\n", expr->const_expr.val);
    break;
  case EXPR_UNARY:
    printf("%*sUnaryOPExpr ", indent, "");
    print_source_range(expr->source_range);
    printf(" operator: %s\n", unary_op_name(expr->unary_op.unary_op_type));
    ast_print_expr(expr->unary_op.inner_expr, indent + 2);
    break;
  case EXPR_BINARY:
    printf("%*sBinaryOPExpr ", indent, "");
    print_source_range(expr->source_range);
    printf(" operator: %s\n", binary_op_name(expr->binary_op.binary_op_type));
    ast_print_expr(expr->binary_op.lhs, indent + 2);
    ast_print_expr(expr->binary_op.rhs, indent + 2);
    break;
  case EXPR_VARIABLE:
    printf("%*sVariableExpr ", indent, "");
    print_source_range(expr->source_range);
    printf(" ");
    print_str(expr->variable->name);
    printf("\n");
    break;
  case EXPR_TERNARY:
    printf("%*sTernaryExpr ", indent, "");
    print_source_range(expr->source_range);
    printf("\n");
    ast_print_expr(expr->ternary.cond, indent + 2);
    ast_print_expr(expr->ternary.true_expr, indent + 2);
    ast_print_expr(expr->ternary.false_expr, indent + 2);
    printf("\n");
    break;
  case EXPR_CALL:
    printf("%*sCallExpr ", indent, "");
    print_source_range(expr->source_range);
    printf("\n");
    ast_print_expr(expr->call.function, indent + 2);
    for (uint32_t i = 0; i < expr->call.arg_count; ++i) {
      ast_print_expr(expr->call.args[i], indent + 2);
    }
    break;
  }
}

static void ast_print_block(const Block* block, int indent);

static const char* string_from_stmt_tag(StmtTag stmt_tag)
{
  switch (stmt_tag) {
  case STMT_INVALID: MCC_UNREACHABLE();
  case STMT_EMPTY: return "EmptyStmt";
  case STMT_EXPR: return "ExprStmt";
  case STMT_COMPOUND: return "CompoundStmt";
  case STMT_RETURN: return "ReturnStmt";
  case STMT_IF: return "IfStmt";
  case STMT_WHILE: return "WhileStmt";
  case STMT_DO_WHILE: return "DoWhileStmt";
  case STMT_FOR: return "ForStmt";
  case STMT_BREAK: return "BreakStmt";
  case STMT_CONTINUE: return "ContinueStmt";
  }
  MCC_UNREACHABLE();
}

static void ast_print_storage_class(StorageClass storage_class)
{
  switch (storage_class) {
  case STORAGE_CLASS_NONE: break;
  case STORAGE_CLASS_EXTERN: printf(" extern"); break;
  case STORAGE_CLASS_STATIC: printf(" static"); break;
  }
}

static void ast_print_var_decl(const VariableDecl* decl, int indent)
{
  printf("%*sVariableDecl ", indent, "");
  print_source_range(decl->source_range);
  printf(" 'int ");
  print_str(decl->name->name);
  printf("'");
  ast_print_storage_class(decl->storage_class);
  printf("\n");
  if (decl->initializer) { ast_print_expr(decl->initializer, indent + 2); }
}

static void ast_print_function_decl(const FunctionDecl* decl, int indent);

static void ast_print_decl(const Decl* decl, int indent)
{
  switch (decl->tag) {
  case DECL_INVALID: MCC_UNREACHABLE(); break;
  case DECL_VAR: ast_print_var_decl(&decl->var, indent); break;
  case DECL_FUNC: ast_print_function_decl(decl->func, indent); break;
  }
}

static void ast_print_nullable_expr(const Expr* expr, int indent)
{
  if (expr != nullptr) {
    ast_print_expr(expr, indent);
  } else {
    printf("%*s<<null>>\n", indent, "");
  }
}

static void ast_print_stmt(const Stmt* stmt, int indent)
{
  printf("%*s%s ", indent, "", string_from_stmt_tag(stmt->tag));
  print_source_range(stmt->source_range);
  printf("\n");

  switch (stmt->tag) {
  case STMT_INVALID: MCC_UNREACHABLE();
  case STMT_EMPTY: break;
  case STMT_EXPR: ast_print_expr(stmt->ret.expr, indent + 2); break;
  case STMT_COMPOUND: ast_print_block(&stmt->compound, indent + 2); break;
  case STMT_RETURN: ast_print_expr(stmt->ret.expr, indent + 2); break;
  case STMT_IF:
    ast_print_expr(stmt->if_then.cond, indent + 2);
    ast_print_stmt(stmt->if_then.then, indent + 2);
    if (stmt->if_then.els != nullptr) {
      ast_print_stmt(stmt->if_then.els, indent + 2);
    }
    break;
  case STMT_WHILE:
    ast_print_expr(stmt->while_loop.cond, indent + 2);
    ast_print_stmt(stmt->while_loop.body, indent + 2);
    break;
  case STMT_DO_WHILE:
    ast_print_stmt(stmt->while_loop.body, indent + 2);
    ast_print_expr(stmt->while_loop.cond, indent + 2);
    break;
  case STMT_FOR:
    switch (stmt->for_loop.init.tag) {
    case FOR_INIT_INVALID: MCC_UNREACHABLE();
    case FOR_INIT_DECL:
      ast_print_var_decl(stmt->for_loop.init.decl, indent + 2);
      break;
    case FOR_INIT_EXPR: {
      ast_print_nullable_expr(stmt->for_loop.init.expr, indent + 2);
    } break;
    }

    ast_print_nullable_expr(stmt->for_loop.cond, indent + 2);
    ast_print_nullable_expr(stmt->for_loop.post, indent + 2);
    ast_print_stmt(stmt->for_loop.body, indent + 2);
    break;
  case STMT_BREAK:
  case STMT_CONTINUE: break;
  }
}

static void ast_print_block_item(const BlockItem* item, int indent)
{
  switch (item->tag) {
  case BLOCK_ITEM_DECL: ast_print_decl(&item->decl, indent); return;
  case BLOCK_ITEM_STMT: ast_print_stmt(&item->stmt, indent); return;
  }
  MCC_UNREACHABLE();
}

static void ast_print_block(const Block* block, int indent)
{
  for (size_t i = 0; i < block->child_count; ++i) {
    ast_print_block_item(&block->children[i], indent);
  }
}

static void ast_print_parameters(Parameters parameters)
{
  if (parameters.length == 0) {
    printf("(void)");
  } else {
    printf("(");
    for (uint32_t i = 0; i < parameters.length; ++i) {
      if (i > 0) { printf(", "); }
      const Identifier* param = parameters.data[i];
      printf("int");
      if (param->name.size != 0) {
        printf(" ");
        print_str(param->name);
      }
    }
    printf(")");
  }
}

static void ast_print_function_decl(const FunctionDecl* decl, int indent)
{
  printf("%*sFunctionDecl ", indent, "");
  print_source_range(decl->source_range);
  printf(" \'int ");
  print_str(decl->name->name);
  ast_print_parameters(decl->params);
  printf("\'");

  ast_print_storage_class(decl->storage_class);
  printf("\n");

  if (decl->body) { ast_print_block(decl->body, indent + 2); }
}

void ast_print_translation_unit(const TranslationUnit* tu)
{
  printf("TranslationUnit\n");
  for (size_t i = 0; i < tu->decl_count; ++i) {
    ast_print_decl(tu->decls + i, 2);
  }
}
