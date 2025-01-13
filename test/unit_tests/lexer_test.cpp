#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <span>

extern "C" {
#include <mcc/arena.h>
#include <mcc/frontend.h>
}

using Catch::Matchers::RangeEquals;

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

  const auto tokens = lex(input, &permanent_arena, scratch_arena);
  const std::span<const TokenType> token_types(tokens.token_types,
                                               tokens.token_count);
  REQUIRE_THAT(expected, RangeEquals(token_types));
}
