#include <ApprovalTests.hpp>
#include <catch2/catch.hpp>
#include <fmt/format.h>

#include <span>
#include <string>
#include <vector>

extern "C" {
#include "lexer.h"
}

#include "source_location_formatter.hpp"

template <> struct fmt::formatter<StringView> : formatter<string_view> {
  template <typename FormatContext>
  auto format(StringView sv, FormatContext& ctx)
  {
    return formatter<string_view>::format(string_view{sv.start, sv.size}, ctx);
  }
};

template <> struct fmt::formatter<TokenType> : formatter<string_view> {
  template <typename FormatContext> auto format(TokenType t, FormatContext& ctx)
  {
    string_view name;

    switch (t) {
    case TOKEN_LEFT_PAREN: name = "("; break;
    case TOKEN_RIGHT_PAREN: name = ")"; break;
    case TOKEN_LEFT_BRACE: name = "{"; break;
    case TOKEN_RIGHT_BRACE: name = "}"; break;
    case TOKEN_SEMICOLON: name = ";"; break;
    case TOKEN_EQUAL: name = "="; break;
    case TOKEN_PLUS: name = "+"; break;
    case TOKEN_MINUS: name = "-"; break;
    case TOKEN_STAR: name = "*"; break;
    case TOKEN_SLASH: name = "/"; break;
    case TOKEN_PERCENT: name = "%"; break;
    case TOKEN_TILDE: name = "~"; break;
    case TOKEN_AMPERSAND: name = "&"; break;
    case TOKEN_BAR: name = "|"; break;
    case TOKEN_LEFT_SHIFT: name = "<<"; break;
    case TOKEN_RIGHT_SHIFT: name = ">>"; break;
    case TOKEN_XOR: name = "^"; break;
    case TOKEN_NOT: name = "!"; break;
    case TOKEN_AMPERSAND_AMPERSAND: name = "&&"; break;
    case TOKEN_BAR_BAR: name = "||"; break;
    case TOKEN_EQUAL_EQUAL: name = "=="; break;
    case TOKEN_NOT_EQUAL: name = "!+"; break;
    case TOKEN_LESS_THAN: name = "<"; break;
    case TOKEN_GREATER_THAN: name = ">"; break;
    case TOKEN_LESS_THAN_OR_EQ: name = "<="; break;
    case TOKEN_GREATER_THAN_OR_EQ: name = ">="; break;
    case TOKEN_KEYWORD_VOID: name = "void"; break;
    case TOKEN_KEYWORD_INT: name = "int"; break;
    case TOKEN_KEYWORD_RETURN: name = "return"; break;
    case TOKEN_IDENTIFIER: name = "IDENTIFIER"; break;
    case TOKEN_INTEGER: name = "INTEGER"; break;
    case TOKEN_ERROR: name = "ERROR"; break;
    case TOKEN_EOF: name = "EOF"; break;
    case TOKEN_TYPES_COUNT: /* shouldn't happen*/ break;
    }

    return formatter<string_view>::format(name, ctx);
  }
};

template <> struct fmt::formatter<Token> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
  {
    if (ctx.begin() != ctx.end())
      throw format_error("invalid format for Token");
    return ctx.begin();
  }

  auto format(Token token, auto& ctx)
  {
    return fmt::format_to(ctx.out(), "{:<8} {} \"{}\"", token.type,
                          token.location, token.src);
  }
};

[[nodiscard]] auto tokenize_source(const char* source) -> std::vector<Token>
{
  std::vector<Token> results;

  Lexer lexer = lexer_create(source);
  while (true) {
    Token token = lexer_scan_token(&lexer);
    results.push_back(token);
    if (token.type == TOKEN_EOF) break;
  }
  return results;
}

[[nodiscard]] auto to_string(std::span<const Token> tokens) -> std::string
{
  return fmt::format("{}", fmt::join(tokens, "\n"));
}

[[nodiscard]] auto verify_scanner(const char* source) -> std::string
{
  return fmt::format("Source:\n{}\n\nTokens:\n{}\n", source,
                     to_string(tokenize_source(source)));
}

TEST_CASE("Lexer test")
{
  SECTION("minimum source")
  {
    constexpr const char* minimum_source = "int main(void)\n"
                                           "{\n"
                                           "  return 42;\n"
                                           "}";
    ApprovalTests::Approvals::verify(verify_scanner(minimum_source));
  }

  SECTION("Arithmetics")
  {
    ApprovalTests::Approvals::verify(verify_scanner("1 + 2 * (3 - 4) / 5"));
  }
}
