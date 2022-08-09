#include <fmt/format.h>

#include "ast_printer.hpp"
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

template <> struct fmt::formatter<StringView> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(StringView sv, FormatContext& ctx)
  {
    string_view name{sv.start, sv.size};
    return formatter<string_view>::format(name, ctx);
  }
};

namespace mcc {

void print_to_string(std::string& buffer, const Expr& expr, int indentation)
{
  indented_format_to(std::back_inserter(buffer), indentation, "Expr {}\n",
                     expr.source_range);
  switch (expr.type) {
  case CONST_EXPR: {
    const auto& const_expr = (const ConstExpr&)expr;
    indented_format_to(std::back_inserter(buffer), indentation + 2, "{}\n",
                       const_expr.val);
    return;
  }
  case BINARY_OP_EXPR:
    // TODO: print binary operators
    return;
  }
}

void print_to_string(std::string& buffer, const Stmt& stmt, int indentation)
{
  switch (stmt.type) {
  case RETURN_STMT:
    indented_format_to(std::back_inserter(buffer), indentation,
                       "ReturnStmt {}\n", stmt.source_range);
    print_to_string(buffer, *((const ReturnStmt&)stmt).expr, indentation + 2);
    break;
  case COMPOUND_STMT:
    indented_format_to(std::back_inserter(buffer), indentation,
                       "CompoundStmt {}\n", stmt.source_range);
    const auto& compound_stmt = (const CompoundStmt&)stmt;
    for (std::size_t i = 0; i < compound_stmt.statement_count; ++i) {
      print_to_string(buffer, *compound_stmt.statements[i], indentation + 2);
    }
    break;
  }
}

void print_to_string(std::string& buffer, const FunctionDecl& decl,
                     int indentation)
{
  indented_format_to(std::back_inserter(buffer), indentation,
                     "FunctionDecl <{}(void) -> int> {}\n", decl.name,
                     decl.source_range);
  if (decl.body) { print_to_string(buffer, decl.body->base, indentation + 2); }
}

} // namespace mcc