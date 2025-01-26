#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <span>

extern "C" {
#include <mcc/frontend.h>
}

#include "arenas.hpp"

using Catch::Matchers::RangeEquals;

TEST_CASE("Lexer lex symbols", "[lexer]")
{
  Arena& permanent_arena = get_permanent_arena();
  const Arena scratch_arena = get_scratch_arena();

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

TEST_CASE("Lexer lex keywords", "[lexer]")
{
  Arena& permanent_arena = get_permanent_arena();
  const Arena scratch_arena = get_scratch_arena();

  static constexpr const char* input = R"(int void return typedef
 if else
 do while for break continue
 let)";

  static constexpr TokenType expected[] = {TOKEN_KEYWORD_INT,
                                           TOKEN_KEYWORD_VOID,
                                           TOKEN_KEYWORD_RETURN,
                                           TOKEN_KEYWORD_TYPEDEF,
                                           TOKEN_KEYWORD_IF,
                                           TOKEN_KEYWORD_ELSE,
                                           TOKEN_KEYWORD_DO,
                                           TOKEN_KEYWORD_WHILE,
                                           TOKEN_KEYWORD_FOR,
                                           TOKEN_KEYWORD_BREAK,
                                           TOKEN_KEYWORD_CONTINUE,
                                           TOKEN_IDENTIFIER,
                                           TOKEN_EOF};

  const auto tokens = lex(input, &permanent_arena, scratch_arena);
  const std::span<const TokenType> token_types(tokens.token_types,
                                               tokens.token_count);
  REQUIRE_THAT(expected, RangeEquals(token_types));
}
