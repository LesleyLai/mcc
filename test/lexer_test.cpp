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
    return formatter<string_view>::format(string_view{sv.start, sv.length},
                                          ctx);
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
    return format_to(ctx.out(), "{} {} \"{}\"", token.type, token.location,
                     token.src);
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