#include <catch2/catch.hpp>

#include <cstdint>

extern "C" {
#include "utils/allocators.h"
}

TEST_CASE("Arena allocation")
{
  constexpr auto size = 100;
  std::uint8_t buffer[size];

  Arena arena = arena_init(buffer, size);
  REQUIRE(buffer == arena.begin);
  REQUIRE(buffer == arena.ptr);

  {
    auto* p = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint8_t));
    *p = 42;
    REQUIRE(p == buffer);

    auto* p2 = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint32_t));
    REQUIRE(p2 == buffer + sizeof(uint32_t));

    REQUIRE(arena.size_remain == size - 2 * sizeof(uint32_t));

    auto* p3 = static_cast<uint8_t*>(arena_aligned_alloc(
        &arena, alignof(std::max_align_t), arena.size_remain + 1));
    REQUIRE(p3 == nullptr);
  }

  arena_reset(&arena);
  REQUIRE(buffer == arena.begin);
  REQUIRE(buffer == arena.ptr);
  REQUIRE(size == arena.size_remain);

  {
    auto* p = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint8_t));
    *p = 42;
    REQUIRE(p == buffer);
  }
}

void test_poly_allocator(PolyAllocator poly_allocator)
{
  auto* p = static_cast<uint8_t*>(POLY_ALLOC_OBJECT(&poly_allocator, uint8_t));
  *p = 42;
  REQUIRE(p != NULL);

  auto* p2 =
      static_cast<uint8_t*>(POLY_ALLOC_OBJECT(&poly_allocator, uint32_t));
  REQUIRE(p2 == p + sizeof(uint32_t));

  auto* p3 =
      poly_aligned_alloc(&poly_allocator, alignof(std::max_align_t), 200);
  REQUIRE(p3 == nullptr);
}

TEST_CASE("PolyAllocator arena allocation")
{
  constexpr auto size = 100;
  std::uint8_t buffer[size];

  Arena arena = arena_init(buffer, size);
  PolyAllocator poly_allocator = poly_allocator_from_arena(&arena);

  test_poly_allocator(poly_allocator);
}