#include <mcc/ast.h>
#include <mcc/prelude.h>

static void print_source_range(SourceRange range)
{
  if (range.begin.line != range.end.line) {
    printf("lines: %d..%d", range.begin.line, range.end.line);
  } else {
    printf("line: %d column: %d..%d", //
           range.begin.line, range.begin.column, range.end.column);
  }
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
  }
  MCC_ASSERT_MSG(false, "invalid enum");
}

static void ast_print_expr(Expr* expr, int indent)
{
  switch (expr->type) {
  case EXPR_INVALID: MCC_UNREACHABLE();
  case EXPR_CONST: printf("%*s%i\n", indent, "", expr->const_expr.val); break;
  case EXPR_UNARY:
    printf("%*sUnaryOPExpr <", indent, "");
    print_source_range(expr->source_range);
    printf("> operator: %s\n", unary_op_name(expr->unary_op.unary_op_type));
    ast_print_expr(expr->unary_op.inner_expr, indent + 2);
    break;
  case EXPR_BINARY:
    printf("%*sBinaryOPExpr <", indent, "");
    print_source_range(expr->source_range);
    printf("> operator: %s\n", binary_op_name(expr->binary_op.binary_op_type));
    ast_print_expr(expr->binary_op.lhs, indent + 2);
    ast_print_expr(expr->binary_op.rhs, indent + 2);
    break;
  }
}

static void ast_print_compound_stmt(CompoundStmt* stmt, int indent);

static void ast_print_stmt(Stmt* stmt, int indent)
{
  switch (stmt->type) {
  case STMT_INVALID: MCC_UNREACHABLE();
  case STMT_COMPOUND:
    ast_print_compound_stmt(&stmt->compound, indent + 2);
    break;
  case STMT_RETURN: {
    printf("%*sReturnStmt <", indent, "");
    print_source_range(stmt->source_range);
    printf(">\n");
    ast_print_expr(stmt->ret.expr, indent + 2);
  } break;
  }
}

static void ast_print_compound_stmt(CompoundStmt* stmt, int indent)
{
  for (size_t i = 0; i < stmt->statement_count; ++i) {
    ast_print_stmt(&stmt->statements[i], indent);
  }
}

static void ast_print_function_decl(FunctionDecl* decl, int indent)
{
  printf("%*sFunctionDecl <", indent, "");
  print_source_range(decl->source_range);
  printf("> \"int %.*s(void)\"\n", (int)decl->name.size, decl->name.start);
  ast_print_compound_stmt(decl->body, indent + 2);
}

void ast_print_translation_unit(TranslationUnit* tu)
{
  printf("TranslationUnit\n");
  for (size_t i = 0; i < tu->decl_count; ++i) {
    ast_print_function_decl(&tu->decls[i], 2);
  }
}
