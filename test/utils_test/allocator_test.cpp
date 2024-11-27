#include <catch2/catch_test_macros.hpp>

#include <cstdint>

extern "C" {
#include "utils/allocators.h"
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
    auto* p = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint8_t));
    *p = 42;
    REQUIRE(p == buffer);
    REQUIRE(arena.previous == buffer);
    REQUIRE(arena.current == buffer + 1);
  }

  {
    auto* p2 = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint32_t));
    REQUIRE(p2 == buffer + sizeof(uint32_t));
    REQUIRE(arena.previous == buffer + 1);
    REQUIRE(arena.current == buffer + 8);
    REQUIRE(arena.size_remain == size - 2 * sizeof(uint32_t));
  }

  {
    auto* p3 = static_cast<uint8_t*>(arena_aligned_alloc(
        &arena, alignof(std::max_align_t), arena.size_remain + 1));
    REQUIRE(p3 == nullptr);
    REQUIRE(arena.previous == buffer + 1);
    REQUIRE(arena.current == buffer + 8);
  }

  arena_reset(&arena);
  REQUIRE(buffer == arena.begin);
  REQUIRE(buffer == arena.current);
  REQUIRE(NULL == arena.previous);
  REQUIRE(size == arena.size_remain);

  {
    auto* p = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint8_t));
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

  auto* p0 = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint8_t));
  p0[0] = 'x';
  auto* p1 = static_cast<uint8_t*>(
      ARENA_ALLOC_ARRAY(&arena, uint8_t, second_alloc_size));
  p1[0] = '4';
  p1[1] = '2';

  SECTION("Inplace grow")
  {
    static constexpr std::size_t new_alloc_size = 32;
    auto* p2 = static_cast<uint8_t*>(
        ARENA_GROW_ARRAY(&arena, uint8_t, p1, new_alloc_size));
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

  SECTION("Calling grow where the pointer is not arena's last allocation "
          "returns NULL")
  {
    static constexpr std::size_t new_alloc_size = 8;
    auto* p2 = static_cast<uint8_t*>(
        ARENA_GROW_ARRAY(&arena, uint8_t, p0, new_alloc_size));
    REQUIRE(p2 == nullptr);
    REQUIRE(p1[0] == '4');
    REQUIRE(p1[1] == '2');
    REQUIRE(arena.current == buffer + total_old_alloc_size);
  }

  SECTION("Calling grow when the arena doesn't have enough memory should "
          "returns NULL")
  {
    auto* p2 =
        static_cast<uint8_t*>(ARENA_GROW_ARRAY(&arena, uint8_t, p1, 200));
    REQUIRE(p2 == nullptr);
    REQUIRE(p0[0] == 'x');
    REQUIRE(p1[0] == '4');
    REQUIRE(p1[1] == '2');
    REQUIRE(arena.previous == buffer + first_alloc_size);
    REQUIRE(arena.current == buffer + total_old_alloc_size);
  }
}

TEST_CASE("Arena shrinking")
{
  constexpr auto size = 100;
  std::uint8_t buffer[size] = {};

  Arena arena = arena_init(buffer, size);

  static constexpr std::size_t first_alloc_size = 1;
  static constexpr std::size_t second_alloc_size = 10;
  static constexpr std::size_t total_old_alloc_size =
      first_alloc_size + second_alloc_size;

  auto* p0 = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint8_t));
  p0[0] = 'x';
  auto* p1 = static_cast<uint8_t*>(
      ARENA_ALLOC_ARRAY(&arena, uint8_t, second_alloc_size));
  for (std::size_t i = 0; i < second_alloc_size; ++i) {
    p1[i] = '0' + static_cast<std::uint8_t>(i);
  }

  SECTION("Shrinking")
  {
    static constexpr std::size_t new_alloc_size = 5;
    auto* p2 = static_cast<uint8_t*>(
        ARENA_SHRINK_ARRAY(&arena, uint8_t, p1, new_alloc_size));
    REQUIRE(p2 == p1);
    REQUIRE(arena.current == buffer + first_alloc_size + new_alloc_size);
    for (std::size_t i = 0; i < new_alloc_size; ++i) {
      REQUIRE(p2[i] == '0' + static_cast<std::uint8_t>(i));
    }
  }

  SECTION("Calling shrink with mismatched alignment returns NULL")
  {
    static constexpr std::size_t new_alloc_size = 2;
    auto* p2 = static_cast<uint8_t*>(
        arena_aligned_shrink(&arena, p1, 2, new_alloc_size));
    REQUIRE(p2 == nullptr);
    for (std::size_t i = 0; i < second_alloc_size; ++i) {
      REQUIRE(p1[i] == '0' + static_cast<std::uint8_t>(i));
    }
    REQUIRE(arena.current == buffer + total_old_alloc_size);
  }

  SECTION("Calling shrink where the pointer is not arena's last allocation "
          "returns NULL")
  {
    static constexpr std::size_t new_alloc_size = 2;
    auto* p2 = static_cast<uint8_t*>(
        ARENA_SHRINK_ARRAY(&arena, uint8_t, p0, new_alloc_size));
    REQUIRE(p2 == nullptr);
    for (std::size_t i = 0; i < second_alloc_size; ++i) {
      REQUIRE(p1[i] == '0' + static_cast<std::uint8_t>(i));
    }
    REQUIRE(arena.current == buffer + total_old_alloc_size);
  }
}
