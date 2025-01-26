#include <mcc/ast.h>
#include <mcc/prelude.h>

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
    print_str(expr->variable);
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
  }
}

static void ast_print_block(const Block* block, int indent);

static void ast_print_stmt(const Stmt* stmt, int indent)
{
  switch (stmt->tag) {
  case STMT_INVALID: MCC_UNREACHABLE();
  case STMT_EMPTY: {
    printf("%*sEmptyStmt ", indent, "");
    print_source_range(stmt->source_range);
    printf("\n");
  } break;
  case STMT_EXPR: {
    printf("%*sExprStmt ", indent, "");
    print_source_range(stmt->source_range);
    printf("\n");
    ast_print_expr(stmt->ret.expr, indent + 2);
  } break;
  case STMT_COMPOUND: {
    printf("%*sCompoundStmt ", indent, "");
    print_source_range(stmt->source_range);
    printf("\n");
    ast_print_block(&stmt->compound, indent + 2);
    break;
  }
  case STMT_RETURN: {
    printf("%*sReturnStmt ", indent, "");
    print_source_range(stmt->source_range);
    printf("\n");
    ast_print_expr(stmt->ret.expr, indent + 2);
  } break;
  case STMT_IF: {
    printf("%*sIfStmt ", indent, "");
    print_source_range(stmt->source_range);
    printf("\n");
    ast_print_expr(stmt->if_then.cond, indent + 2);
    ast_print_stmt(stmt->if_then.then, indent + 2);
    if (stmt->if_then.els != nullptr) {
      ast_print_stmt(stmt->if_then.els, indent + 2);
    }
  } break;
  case STMT_WHILE: {
    printf("%*sWhileStmt ", indent, "");
    print_source_range(stmt->source_range);
    printf("\n");
    ast_print_expr(stmt->while_loop.cond, indent + 2);
    ast_print_stmt(stmt->while_loop.body, indent + 2);
  } break;
  case STMT_DO_WHILE: MCC_UNIMPLEMENTED(); break;
  case STMT_FOR: MCC_UNIMPLEMENTED(); break;
  case STMT_BREAK: MCC_UNIMPLEMENTED(); break;
  case STMT_CONTINUE: MCC_UNIMPLEMENTED(); break;
  }
}

static void ast_print_block_item(const BlockItem* item, int indent)
{
  switch (item->tag) {
  case BLOCK_ITEM_DECL: {
    printf("%*sVariableDecl ", indent, "");
    printf("int ");
    print_str(item->decl.name);
    printf("\n");
    if (item->decl.initializer) {
      ast_print_expr(item->decl.initializer, indent + 2);
    }
  }
    return;
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

static void ast_print_function_decl(const FunctionDecl* decl, int indent)
{
  printf("%*sFunctionDecl ", indent, "");
  print_source_range(decl->source_range);
  printf(" \"int ");
  print_str(decl->name);
  printf("(void)\"\n");
  ast_print_block(decl->body, indent + 2);
}

void ast_print_translation_unit(const TranslationUnit* tu)
{
  printf("TranslationUnit\n");
  for (size_t i = 0; i < tu->decl_count; ++i) {
    ast_print_function_decl(&tu->decls[i], 2);
  }
}
