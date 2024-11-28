#include "ast.h"
#include "utils/prelude.h"
#include <stdio.h>

static void print_source_range(SourceRange range)
{
  if (range.begin.line != range.end.line) {
    printf("lines: %d..%d", range.begin.line, range.end.line);
  } else {
    printf("line: %d column: %d..%d", //
           range.begin.line, range.begin.column, range.end.column);
  }
}

static void ast_print_expr(Expr* expr, int indent)
{
  switch (expr->type) {
  case CONST_EXPR: printf("%*s%i\n", indent, "", expr->const_expr.val); break;
  case BINARY_OP_EXPR:
    // TODO
    break;
  }
}

static void ast_print_compound_stmt(CompoundStmt* stmt, int indent);

static void ast_print_stmt(Stmt* stmt, int indent)
{
  switch (stmt->type) {
  case COMPOUND_STMT:
    ast_print_compound_stmt(&stmt->compound, indent + 2);
    break;
  case RETURN_STMT: {
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
