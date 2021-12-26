#include <catch2/catch.hpp>
#include <fmt/format.h>

#include <cstdint>

extern "C" {
#include "arena.h"
}

TEST_CASE("Arena allocator")
{
  constexpr auto size = 100;
  std::uint8_t buffer[size];

  Arena arena = arena_init(buffer, size);
  REQUIRE(buffer == arena.begin);
  REQUIRE(buffer == arena.ptr);

  auto* p = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint8_t));
  *p = 42;
  REQUIRE(p == buffer);

  auto* p2 = static_cast<uint8_t*>(ARENA_ALLOC_OBJECT(&arena, uint32_t));
  REQUIRE(p2 == buffer + sizeof(uint32_t));

  REQUIRE(arena.size_remain == size - 2 * sizeof(uint32_t));

  auto* p3 = static_cast<uint8_t*>(arena_aligned_alloc(
      &arena, alignof(std::max_align_t), arena.size_remain + 1));
  REQUIRE(p3 == nullptr);

  auto* p4 = static_cast<uint8_t*>(arena_aligned_alloc(
      &arena, alignof(std::max_align_t), arena.size_remain));
  REQUIRE(p4 ==
          buffer + std::max(2 * sizeof(uint32_t), alignof(std::max_align_t)));

  REQUIRE(arena.size_remain == 0);
}