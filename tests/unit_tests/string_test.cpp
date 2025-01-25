#include <catch2/catch_test_macros.hpp>

extern "C" {
#include <mcc/format.h>
#include <mcc/str.h>
}

#include <fmt/format.h>

using namespace std::string_view_literals;

TEST_CASE("String View")
{
  SECTION("str_eq")
  {
    REQUIRE(not str_eq(str("0"), str("012")));
    REQUIRE(str_eq(str("123"), str("123")));
    REQUIRE(not str_eq(str("123"), str("124")));
  }

  SECTION("str_start_with")
  {
    REQUIRE(not str_start_with(str("0"), str("012")));
    REQUIRE(str_start_with(str("2134"), str("21")));
    REQUIRE(not str_start_with(str("234"), str("21")));
  }
}

TEST_CASE("String Buffer")
{
  constexpr auto buffer_size = 1000;
  std::uint8_t buffer[buffer_size];

  Arena arena = arena_init(buffer, buffer_size);

  SECTION("String buffer push")
  {
    auto string = string_buffer_new(&arena);
    REQUIRE(string_buffer_size(string) == 0);

    string_buffer_push(&string, 'a');
    REQUIRE(string_buffer_size(string) == 1);
    REQUIRE(string_buffer_data(&string)[0] == 'a');
    REQUIRE(str_eq(str_from_buffer(&string), str("a")));
    REQUIRE(!str_eq(str_from_buffer(&string), str("")));
  }

  SECTION("StringBuffer can grow from small to large")
  {
    auto string = string_buffer_new(&arena);
    std::string expected;

    static constexpr std::size_t size = 30;
    for (std::size_t i = 0; i < size; ++i) {
      string_buffer_push(&string, 'b');
      expected.push_back('b');

      REQUIRE(string_buffer_size(string) == expected.size());
      REQUIRE(string_buffer_data(&string) == expected);
    }

    REQUIRE(string_buffer_capacity(string) >= size);
    REQUIRE(string_buffer_size(string) == size);
  }

  SECTION("Append to StringBuffer")
  {
    SECTION("Small to small")
    {
      auto string = string_buffer_from_c_str("Hello, ", &arena);
      string_buffer_append(&string, str("world!"));

      REQUIRE(str_eq(str_from_buffer(&string), str("Hello, world!")));
    }

    SECTION("Small to large")
    {
      auto string = string_buffer_from_c_str("Hello ", &arena);
      string_buffer_append(&string,
                           str("from this really really really weird string!"));

      REQUIRE(
          str_eq(str_from_buffer(&string),
                 str("Hello from this really really really weird string!")));
    }

    SECTION("Large to large")
    {
      auto string = string_buffer_from_c_str(
          "Hello from this really really really ", &arena);
      string_buffer_append(&string, str("weird string!"));

      REQUIRE(
          str_eq(str_from_buffer(&string),
                 str("Hello from this really really really weird string!")));
    }
  }
}
