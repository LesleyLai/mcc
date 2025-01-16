#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <span>

extern "C" {
#include <mcc/arena.h>
#include <mcc/frontend.h>
}

#include "arenas.hpp"

using Catch::Matchers::RangeEquals;

TEST_CASE("Line numbers calculation", "[lexer]")
{
  const StringView src = str(R"(int main(void)
{
  return 0;
}
)");

  const uint32_t expected_line_starts[] = {0, 15, 17, 29, 31};

  const auto* linum_table = get_line_num_table(
      "test.c", src, &get_permanent_arena(), get_scratch_arena());

  const std::span<const uint32_t> line_starts(linum_table->line_starts,
                                              linum_table->line_count);
  REQUIRE_THAT(expected_line_starts, RangeEquals(line_starts));

  SECTION("find line and column for the first character")
  {
    static constexpr uint32_t offset = 0;
    const auto line_column = calculate_line_and_column(linum_table, offset);
    REQUIRE(line_column.line == 1);
    REQUIRE(line_column.column == 1);
  }

  SECTION("find line and column for the first character of return")
  {
    static constexpr uint32_t offset = 19;
    REQUIRE(src.start[offset] == 'r');
    const auto line_column = calculate_line_and_column(linum_table, offset);
    REQUIRE(line_column.line == 3);
    REQUIRE(line_column.column == 3);
  }
}