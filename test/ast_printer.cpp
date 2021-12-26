#include <fmt/format.h>

#include "ast_printer.hpp"

namespace {

template <typename OutputItr, typename... Args>
void indented_format_to(OutputItr out, int indent, std::string_view format_str,
                        Args&&... args)
{
  fmt::format_to(out, "{:{}}", "", indent);
  fmt::format_to(out, format_str, std::forward<Args>(args)...);
}

} // anonymous namespace

template <> struct fmt::formatter<StringView> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(StringView sv, FormatContext& ctx)
  {
    string_view name{sv.start, sv.length};
    return formatter<string_view>::format(name, ctx);
  }
};

namespace mcc {

void print_to_string(std::string& buffer, const Expr& expr, int indentation)
{
  indented_format_to(std::back_inserter(buffer), indentation, "Expr\n");
  indented_format_to(std::back_inserter(buffer), indentation + 2, "{}\n",
                     expr.val);
}

void print_to_string(std::string& buffer, const Stmt& stmt, int indentation)
{
  switch (stmt.type) {
  case RETURN_STMT:
    indented_format_to(std::back_inserter(buffer), indentation, "ReturnStmt\n");
    indented_format_to(std::back_inserter(buffer), indentation + 2, "{}\n",
                       stmt.return_statement.expr->val);
    return;
  case COMPOUND_STMT:
    return print_to_string(buffer, stmt.compound_statement, indentation);
  }

  indented_format_to(std::back_inserter(buffer), indentation, "CompoundStmt\n");
}

void print_to_string(std::string& buffer, const CompoundStmt& stmt,
                     int indentation)
{
  indented_format_to(std::back_inserter(buffer), indentation, "CompoundStmt\n");
  for (std::size_t i = 0; i < stmt.statement_count; ++i) {
    print_to_string(buffer, *stmt.statements[i], indentation + 2);
  }
}

void print_to_string(std::string& buffer, const FunctionDecl& decl,
                     int indentation)
{
  indented_format_to(std::back_inserter(buffer), indentation,
                     "FunctionDecl <{} ()>\n", decl.name);
  if (decl.body) { print_to_string(buffer, *decl.body, indentation + 2); }
}

} // namespace mcc