#include <catch2/catch.hpp>

extern "C" {
#include "utils/str.h"
}

#include <fmt/format.h>

TEST_CASE("String Buffer")
{
  constexpr auto buffer_size = 1000;
  std::uint8_t buffer[buffer_size];

  Arena arena = arena_init(buffer, buffer_size);
  PolyAllocator poly_allocator = poly_allocator_from_arena(&arena);

  SECTION("String buffer push")
  {
    auto string = string_buffer_new(&poly_allocator);
    REQUIRE(string_buffer_size(string) == 0);

    string_buffer_push(&string, 'a');
    REQUIRE(string_buffer_capacity(string) == small_string_capacity);
    REQUIRE(string_buffer_size(string) == 1);
    REQUIRE(string_buffer_data(&string)[0] == 'a');
    REQUIRE(string_view_eq(string_view_from_buffer(&string),
                           string_view_from_c_str("a")));
    REQUIRE(!string_view_eq(string_view_from_buffer(&string),
                            string_view_from_c_str("")));
  }

  SECTION("StringBuffer can grow from small to large")
  {
    auto string = string_buffer_new(&poly_allocator);
    std::string expected;

    static constexpr std::size_t size = 30;
    static_assert(size > small_string_capacity);
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
      auto string = string_buffer_from_c_str("Hello, ", &poly_allocator);
      string_buffer_append(&string, string_view_from_c_str("world!"));

      //      auto sv = string_view_from_c_str("Hello, world!");
      //      fmt::print("{}\n", sv.size);
      //      fmt::print("{}\n", string_buffer_size(string));
      //
      //      for (int i = 0; i < 13; ++i) {
      //        fmt::print("{}:{}\n", sv.start[i],
      //        string_buffer_data(&string)[i]);
      //      }

      REQUIRE(string_view_eq(string_view_from_buffer(&string),
                             string_view_from_c_str("Hello, world!")));
    }

    SECTION("Small to large")
    {
      auto string = string_buffer_from_c_str("Hello ", &poly_allocator);
      string_buffer_append(&string,
                           string_view_from_c_str(
                               "from this really really really weird string!"));

      REQUIRE(string_view_eq(
          string_view_from_buffer(&string),
          string_view_from_c_str(
              "Hello from this really really really weird string!")));
    }

    SECTION("Large to large")
    {
      auto string = string_buffer_from_c_str(
          "Hello from this really really really ", &poly_allocator);
      string_buffer_append(&string, string_view_from_c_str("weird string!"));

      REQUIRE(string_view_eq(
          string_view_from_buffer(&string),
          string_view_from_c_str(
              "Hello from this really really really weird string!")));
    }
  }
}
