
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <ranges>

extern "C" {
#include <mcc/arena.h>
#include <mcc/parser.h>
}

namespace stdr = std::ranges;
namespace stdv = std::views;

class TokensView : public std::ranges::view_interface<TokensView> {
public:
  TokensView() = default;
  explicit TokensView(Tokens tokens) : begin_(tokens.begin), end_(tokens.end) {}

  [[nodiscard]] auto begin() const { return begin_; }
  [[nodiscard]] auto end() const { return end_; }

private:
  Token* begin_;
  Token* end_;
};

TEST_CASE("Lexer lex symbols", "[lexer]")
{
  Arena permanent_arena = arena_from_virtual_mem(4096);
  const Arena scratch_arena = arena_from_virtual_mem(4096);

  static constexpr const char* input = R"(= == != < << <= > >> >=
& && &= | || |= ! ^ ^= <<= >>=
+ ++ += - -- -= -> * *= / /= % %=
, . ?:;)";

  static constexpr TokenType expected[] = {TOKEN_EQUAL,
                                           TOKEN_EQUAL_EQUAL,
                                           TOKEN_NOT_EQUAL,
                                           TOKEN_LESS,
                                           TOKEN_LESS_LESS,
                                           TOKEN_LESS_EQUAL,
                                           TOKEN_GREATER,
                                           TOKEN_GREATER_GREATER,
                                           TOKEN_GREATER_EQUAL,
                                           TOKEN_AMPERSAND,
                                           TOKEN_AMPERSAND_AMPERSAND,
                                           TOKEN_AMPERSAND_EQUAL,
                                           TOKEN_BAR,
                                           TOKEN_BAR_BAR,
                                           TOKEN_BAR_EQUAL,
                                           TOKEN_NOT,
                                           TOKEN_CARET,
                                           TOKEN_CARET_EQUAL,
                                           TOKEN_LESS_LESS_EQUAL,
                                           TOKEN_GREATER_GREATER_EQUAL,
                                           TOKEN_PLUS,
                                           TOKEN_PLUS_PLUS,
                                           TOKEN_PLUS_EQUAL,
                                           TOKEN_MINUS,
                                           TOKEN_MINUS_MINUS,
                                           TOKEN_MINUS_EQUAL,
                                           TOKEN_MINUS_GREATER,
                                           TOKEN_STAR,
                                           TOKEN_STAR_EQUAL,
                                           TOKEN_SLASH,
                                           TOKEN_SLASH_EQUAL,
                                           TOKEN_PERCENT,
                                           TOKEN_PERCENT_EQUAL,
                                           TOKEN_COMMA,
                                           TOKEN_DOT,
                                           TOKEN_QUESTION,
                                           TOKEN_COLON,
                                           TOKEN_SEMICOLON,
                                           TOKEN_EOF};

  const TokensView tokens{lex(input, &permanent_arena, scratch_arena)};
  REQUIRE_THAT(stdv::transform(tokens, &Token::type),
               Catch::Matchers::RangeEquals(expected));
}
