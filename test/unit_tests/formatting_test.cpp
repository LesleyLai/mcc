#include <catch2/catch_test_macros.hpp>

extern "C" {
#include "utils/format.h"
#include "utils/str.h"
}

#include <cstdint>
#include <string_view>

extern "C" {
#include "utils/arena.h"
#include "utils/format.h"
#include "utils/str.h"
}

using namespace std::string_view_literals;

TEST_CASE("Allocate Formatting", "[format]")
{
  constexpr auto buffer_size = 1000;
  std::uint8_t buffer[buffer_size];

  Arena arena = arena_init(buffer, buffer_size);
  const char* result = allocate_printf(&arena, "Hello, %s!", "world");

  const auto expected = "Hello, world!"sv;
  REQUIRE(result == expected);

  REQUIRE(arena.current - buffer == static_cast<intptr_t>(expected.size() + 1));
}

TEST_CASE("String Buffer formatting", "[format]")
{
  constexpr auto buffer_size = 1000;
  std::uint8_t buffer[buffer_size];

  Arena arena = arena_init(buffer, buffer_size);

  SECTION("Formatting to a new string")
  {
    StringBuffer sb = string_buffer_new(&arena);
    string_buffer_printf(&sb, "Hello, %s in %d!", "world", 2022);
    REQUIRE(string_buffer_c_str(&sb) == "Hello, world in 2022!"sv);
  }

  SECTION("Append formatted string to an existing string")
  {
    StringBuffer sb = string_buffer_from_c_str("Hello, ", &arena);
    string_buffer_printf(&sb, "%s in %d!", "world", 2022);
    REQUIRE(string_buffer_c_str(&sb) == "Hello, world in 2022!"sv);
  }

  SECTION("Append large formatted string to an existing string")
  {
    StringBuffer sb = string_buffer_from_c_str("Hello, ", &arena);
    string_buffer_printf(&sb, "%s in %d! %s", "world", 2022, "La la la la!");
    REQUIRE(string_buffer_c_str(&sb) == "Hello, world in 2022! La la la la!"sv);

    string_buffer_printf(&sb, " %s", "La la la!");
    REQUIRE(string_buffer_c_str(&sb) ==
            "Hello, world in 2022! La la la la! La la la!"sv);
  }
}