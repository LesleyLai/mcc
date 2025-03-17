#include <mcc/ast.h>
#include <mcc/format.h>
#include <mcc/prelude.h>

#include "symbol_table.h"

static void format_source_range(StringBuffer* output, SourceRange range)
{
  // TODO: print line and columns
  string_buffer_printf(output, "<%d..%d>", range.begin, range.end);
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

static void format_expr(StringBuffer* output, const Expr* expr, int indent)
{
  switch (expr->tag) {
  case EXPR_INVALID: MCC_UNREACHABLE();
  case EXPR_CONST:
    string_buffer_printf(output, "%*sIntegerLiteral ", indent, "");
    format_source_range(output, expr->source_range);
    string_buffer_printf(output, " %i\n", expr->const_expr.val);
    break;
  case EXPR_UNARY:
    string_buffer_printf(output, "%*sUnaryOPExpr ", indent, "");
    format_source_range(output, expr->source_range);
    string_buffer_printf(output, " operator: %s\n",
                         unary_op_name(expr->unary_op.unary_op_type));
    format_expr(output, expr->unary_op.inner_expr, indent + 2);
    break;
  case EXPR_BINARY:
    string_buffer_printf(output, "%*sBinaryOPExpr ", indent, "");
    format_source_range(output, expr->source_range);
    string_buffer_printf(output, " operator: %s\n",
                         binary_op_name(expr->binary_op.binary_op_type));
    format_expr(output, expr->binary_op.lhs, indent + 2);
    format_expr(output, expr->binary_op.rhs, indent + 2);
    break;
  case EXPR_VARIABLE:
    string_buffer_printf(output, "%*sVariableExpr ", indent, "");
    format_source_range(output, expr->source_range);
    string_buffer_printf(output, " %.*s\n", (int)expr->variable->name.size,
                         expr->variable->name.start);
    break;
  case EXPR_TERNARY:
    string_buffer_printf(output, "%*sTernaryExpr ", indent, "");
    format_source_range(output, expr->source_range);
    string_buffer_printf(output, "\n");
    format_expr(output, expr->ternary.cond, indent + 2);
    format_expr(output, expr->ternary.true_expr, indent + 2);
    format_expr(output, expr->ternary.false_expr, indent + 2);
    string_buffer_printf(output, "\n");
    break;
  case EXPR_CALL:
    string_buffer_printf(output, "%*sCallExpr ", indent, "");
    format_source_range(output, expr->source_range);
    string_buffer_printf(output, "\n");
    format_expr(output, expr->call.function, indent + 2);
    for (uint32_t i = 0; i < expr->call.arg_count; ++i) {
      format_expr(output, expr->call.args[i], indent + 2);
    }
    break;
  }
}

static void format_blocks(StringBuffer* output, const Block* block, int indent);

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

static void format_storage_class(StringBuffer* output,
                                 StorageClass storage_class)
{
  switch (storage_class) {
  case STORAGE_CLASS_NONE: break;
  case STORAGE_CLASS_EXTERN:
    string_buffer_append(output, str("extern "));
    break;
  case STORAGE_CLASS_STATIC:
    string_buffer_append(output, str("static "));
    break;
  }
}

static void format_var_decl(StringBuffer* output, const VariableDecl* decl,
                            int indent)
{
  string_buffer_printf(output, "%*sVariableDecl ", indent, "");
  format_source_range(output, decl->source_range);
  string_buffer_printf(output, " %.*s: ", (int)decl->identifier->name.size,
                       decl->identifier->name.start);
  format_storage_class(output, decl->storage_class);
  string_buffer_append(output, str("int\n"));
  if (decl->initializer) { format_expr(output, decl->initializer, indent + 2); }
}

static void format_function_decl(StringBuffer* output, const FunctionDecl* decl,
                                 int indent);

static void format_decl(StringBuffer* output, const Decl* decl, int indent)
{
  switch (decl->tag) {
  case DECL_INVALID: MCC_UNREACHABLE(); break;
  case DECL_VAR: format_var_decl(output, &decl->var, indent); break;
  case DECL_FUNC: format_function_decl(output, decl->func, indent); break;
  }
}

static void format_nullable_expr(StringBuffer* output, const Expr* expr,
                                 int indent)
{
  if (expr != nullptr) {
    format_expr(output, expr, indent);
  } else {
    string_buffer_printf(output, "%*s<<null>>\n", indent, "");
  }
}

static void format_stmt(StringBuffer* output, const Stmt* stmt, int indent)
{
  string_buffer_printf(output, "%*s%s ", indent, "",
                       string_from_stmt_tag(stmt->tag));
  format_source_range(output, stmt->source_range);
  string_buffer_append(output, str("\n"));

  switch (stmt->tag) {
  case STMT_INVALID: MCC_UNREACHABLE();
  case STMT_EMPTY: break;
  case STMT_EXPR: format_expr(output, stmt->ret.expr, indent + 2); break;
  case STMT_COMPOUND: format_blocks(output, &stmt->compound, indent + 2); break;
  case STMT_RETURN: format_expr(output, stmt->ret.expr, indent + 2); break;
  case STMT_IF:
    format_expr(output, stmt->if_then.cond, indent + 2);
    format_stmt(output, stmt->if_then.then, indent + 2);
    if (stmt->if_then.els != nullptr) {
      format_stmt(output, stmt->if_then.els, indent + 2);
    }
    break;
  case STMT_WHILE:
    format_expr(output, stmt->while_loop.cond, indent + 2);
    format_stmt(output, stmt->while_loop.body, indent + 2);
    break;
  case STMT_DO_WHILE:
    format_stmt(output, stmt->while_loop.body, indent + 2);
    format_expr(output, stmt->while_loop.cond, indent + 2);
    break;
  case STMT_FOR:
    switch (stmt->for_loop.init.tag) {
    case FOR_INIT_INVALID: MCC_UNREACHABLE();
    case FOR_INIT_DECL:
      format_var_decl(output, stmt->for_loop.init.decl, indent + 2);
      break;
    case FOR_INIT_EXPR: {
      format_nullable_expr(output, stmt->for_loop.init.expr, indent + 2);
    } break;
    }

    format_nullable_expr(output, stmt->for_loop.cond, indent + 2);
    format_nullable_expr(output, stmt->for_loop.post, indent + 2);
    format_stmt(output, stmt->for_loop.body, indent + 2);
    break;
  case STMT_BREAK:
  case STMT_CONTINUE: break;
  }
}

static void format_block_item(StringBuffer* output, const BlockItem* item,
                              int indent)
{
  switch (item->tag) {
  case BLOCK_ITEM_DECL: format_decl(output, &item->decl, indent); return;
  case BLOCK_ITEM_STMT: format_stmt(output, &item->stmt, indent); return;
  }
  MCC_UNREACHABLE();
}

static void format_blocks(StringBuffer* output, const Block* block, int indent)
{
  for (size_t i = 0; i < block->child_count; ++i) {
    format_block_item(output, &block->children[i], indent);
  }
}

static void format_parameters(StringBuffer* output, Parameters parameters)
{
  if (parameters.length == 0) {
    string_buffer_printf(output, "(void)");
  } else {
    string_buffer_printf(output, "(");
    for (uint32_t i = 0; i < parameters.length; ++i) {
      if (i > 0) { string_buffer_printf(output, ", "); }
      const IdentifierInfo* param = parameters.data[i];
      string_buffer_printf(output, "int");
      if (param->name.size != 0) {
        string_buffer_printf(output, " %.*s", (int)param->name.size,
                             param->name.start);
      }
    }
    string_buffer_printf(output, ")");
  }
}

static void format_function_decl(StringBuffer* output, const FunctionDecl* decl,
                                 int indent)
{
  string_buffer_printf(output, "%*sFunctionDecl ", indent, "");
  format_source_range(output, decl->source_range);

  string_buffer_printf(output, " %.*s: ", (int)decl->name->name.size,
                       decl->name->name.start);
  format_storage_class(output, decl->storage_class);
  string_buffer_printf(output, "int");
  format_parameters(output, decl->params);
  string_buffer_printf(output, "\n");

  if (decl->body) { format_blocks(output, decl->body, indent + 2); }
}

StringView string_from_ast(const TranslationUnit* tu, Arena* permanent_arena)
{
  StringBuffer output = string_buffer_new(permanent_arena);
  string_buffer_append(&output, str("TranslationUnit\n"));
  for (size_t i = 0; i < tu->decl_count; ++i) {
    format_decl(&output, tu->decls + i, 2);
  }

  return str_from_buffer(&output);
}
