#include <catch2/catch_test_macros.hpp>

#include <cstdint>

extern "C" {
#include "utils/arena.h"
}

TEST_CASE("Arena allocation")
{
  constexpr auto size = 100;
  std::uint8_t buffer[size] = {};

  Arena arena = arena_init(buffer, size);
  REQUIRE(buffer == arena.begin);
  REQUIRE(buffer == arena.current);
  REQUIRE(nullptr == arena.previous);

  {
    auto* p = ARENA_ALLOC_OBJECT(&arena, uint8_t);
    *p = 42;
    REQUIRE(p == buffer);
    REQUIRE(arena.previous == buffer);
    REQUIRE(arena.current == buffer + 1);
  }

  {
    auto* p2 = ARENA_ALLOC_OBJECT(&arena, uint32_t);
    REQUIRE((void*)p2 == buffer + sizeof(uint32_t));
    REQUIRE(arena.previous == buffer + 1);
    REQUIRE(arena.current == buffer + 8);
    REQUIRE(arena.size_remain == size - 2 * sizeof(uint32_t));
  }

  arena_reset(&arena);
  REQUIRE(buffer == arena.begin);
  REQUIRE(buffer == arena.current);
  REQUIRE(NULL == arena.previous);
  REQUIRE(size == arena.size_remain);

  {
    auto* p = ARENA_ALLOC_OBJECT(&arena, uint8_t);
    *p = 42;
    REQUIRE(p == buffer);
  }
}

TEST_CASE("Arena growing")
{
  constexpr auto size = 100;
  std::uint8_t buffer[size] = {};

  Arena arena = arena_init(buffer, size);

  static constexpr std::size_t first_alloc_size = 1;
  static constexpr std::size_t second_alloc_size = 2;
  static constexpr std::size_t total_old_alloc_size =
      first_alloc_size + second_alloc_size;

  auto* p0 = ARENA_ALLOC_OBJECT(&arena, uint8_t);
  p0[0] = 'x';
  auto* p1 = ARENA_ALLOC_ARRAY(&arena, uint8_t, second_alloc_size);
  p1[0] = '4';
  p1[1] = '2';

  SECTION("Inplace grow")
  {
    static constexpr std::size_t new_alloc_size = 32;
    auto* p2 = ARENA_GROW_ARRAY(&arena, uint8_t, p1, new_alloc_size);
    REQUIRE(p2 == p1);
    REQUIRE(arena.current == buffer + first_alloc_size + new_alloc_size);
    REQUIRE(p2[0] == '4');
    REQUIRE(p2[1] == '2');
  }

  SECTION("Calling grow with mismatched alignment should reallocate")
  {
    static constexpr std::size_t new_alloc_size = 8;
    auto* p2 = static_cast<uint8_t*>(
        arena_aligned_grow(&arena, p1, 4, new_alloc_size));
    REQUIRE(p2 != nullptr);
    REQUIRE(p2 != p1);
    REQUIRE(p2[0] == '4');
    REQUIRE(p2[1] == '2');
    REQUIRE(arena.current ==
            buffer + total_old_alloc_size + 1 + new_alloc_size);
  }
}
