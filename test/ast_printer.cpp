#include <fmt/format.h>

#include "ast_printer.hpp"
#include "cstring_view_format.hpp"
#include "source_location_formatter.hpp"

namespace {

template <typename OutputItr, typename... Args>
void indented_format_to(OutputItr out, int indent, std::string_view format_str,
                        Args&&... args)
{
  fmt::format_to(out, "{:{}}", "", indent);
  fmt::format_to(out, fmt::runtime(format_str), std::forward<Args>(args)...);
}

} // anonymous namespace

namespace mcc {

void print_to_string(std::string& buffer, const Expr& expr, int indentation)
{
  indented_format_to(std::back_inserter(buffer), indentation, "Expr <{}>\n",
                     expr.source_range);
  switch (expr.type) {
  case CONST_EXPR: {
    indented_format_to(std::back_inserter(buffer), indentation + 2, "{}\n",
                       expr.const_expr.val);
    return;
  }
  case BINARY_OP_EXPR:
    // TODO: print binary operators
    return;
  }
}

void print_to_string(std::string& buffer, const CompoundStmt& stmt,
                     int indentation);

void print_to_string(std::string& buffer, const Stmt& stmt, int indentation)
{
  switch (stmt.type) {
  case RETURN_STMT:
    indented_format_to(std::back_inserter(buffer), indentation,
                       "ReturnStmt <{}>\n", stmt.source_range);
    print_to_string(buffer, *stmt.ret.expr, indentation + 2);
    break;
  case COMPOUND_STMT:
    indented_format_to(std::back_inserter(buffer), indentation,
                       "CompoundStmt <{}>\n", stmt.source_range);
    print_to_string(buffer, stmt.compound, indentation + 2);
    break;
  }
}

void print_to_string(std::string& buffer, const CompoundStmt& compound,
                     int indentation)
{
  for (std::size_t i = 0; i < compound.statement_count; ++i) {
    print_to_string(buffer, compound.statements[i], indentation);
  }
}

void print_to_string(std::string& buffer, const FunctionDecl& decl,
                     int indentation)
{
  indented_format_to(std::back_inserter(buffer), indentation,
                     "FunctionDecl <{}> \"int {}(void)\"\n", decl.source_range,
                     decl.name);
  if (decl.body) { print_to_string(buffer, *decl.body, indentation + 2); }
}

void print_to_string(std::string& buffer, const TranslationUnit& tu,
                     int indentation)
{
  indented_format_to(std::back_inserter(buffer), indentation,
                     "TranslationUnit\n");
  for (std::size_t i = 0; i < tu.decl_count; ++i) {
    print_to_string(buffer, tu.decls[i], indentation + 2);
  }
}

} // namespace mcc