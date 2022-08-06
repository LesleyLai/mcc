#include <catch2/catch.hpp>

extern "C" {
#include "utils/str.h"
}

TEST_CASE("String Buffer")
{
  constexpr auto size = 100;
  std::uint8_t buffer[size];

  Arena arena = arena_init(buffer, size);
  PolyAllocator poly_allocator = poly_allocator_from_arena(&arena);

  SECTION("String buffer push")
  {
    auto string = string_buffer_new(&poly_allocator);

    string_buffer_push(&string, 'a');

    REQUIRE(string.length == 1);
    REQUIRE(string.start[0] == 'a');
    REQUIRE(string_view_eq(string_view_from_buffer(string),
                           string_view_from_c_str("a")));
    REQUIRE(!string_view_eq(string_view_from_buffer(string),
                            string_view_from_c_str("")));
  }

  SECTION("StringBuffer can grow multiple times")
  {
    auto string = string_buffer_new(&poly_allocator);

    // 20 is more than the size for second grow (16)
    for (int i = 0; i < 20; ++i) {
      string_buffer_push(&string, 'b');
    }

    REQUIRE(string.capacity >= 20);
    REQUIRE(string.length == 20);
    REQUIRE(string_view_eq(string_view_from_buffer(string),
                           string_view_from_c_str("bbbbbbbbbbbbbbbbbbbb")));
  }

  SECTION("Append to StringBuffer")
  {
    auto string = string_buffer_from_c_str("Hello, ", &poly_allocator);
    string_buffer_append(&string, string_view_from_c_str("world!"));

    REQUIRE(string_view_eq(string_view_from_buffer(string),
                           string_view_from_c_str("Hello, world!")));
  }
}
