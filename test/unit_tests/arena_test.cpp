#include <catch2/catch_test_macros.hpp>

#include <cstdint>

extern "C" {
#include <mcc/arena.h>
}

static void require_ptr_equal(const void* ptr1, const void* ptr2)
{
  REQUIRE(ptr1 == ptr2);
}

TEST_CASE("Arena allocation")
{
  constexpr auto size = 100;
  std::byte buffer[size] = {};

  Arena arena = arena_init(buffer, size);
  require_ptr_equal(buffer, arena.begin);
  require_ptr_equal(buffer, arena.current);
  require_ptr_equal(nullptr, arena.previous);

  {
    auto* p = ARENA_ALLOC_OBJECT(&arena, uint8_t);
    *p = 42;
    require_ptr_equal(p, buffer);
    require_ptr_equal(arena.previous, buffer);
    require_ptr_equal(arena.current, buffer + 1);
  }

  {
    auto* p2 = ARENA_ALLOC_OBJECT(&arena, uint32_t);
    require_ptr_equal(p2, buffer + sizeof(uint32_t));
    require_ptr_equal(arena.previous, buffer + 1);
    require_ptr_equal(arena.current, buffer + 8);
    REQUIRE(arena.size_remain == size - 2 * sizeof(uint32_t));
  }

  arena_reset(&arena);
  require_ptr_equal(buffer, arena.begin);
  require_ptr_equal(buffer, arena.current);
  require_ptr_equal(nullptr, arena.previous);
  REQUIRE(size == arena.size_remain);

  {
    auto* p = ARENA_ALLOC_OBJECT(&arena, uint8_t);
    *p = 42;
    require_ptr_equal(buffer, p);
  }
}

TEST_CASE("Arena realloc")
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

  SECTION("realloc without a previous allocation")
  {
    static constexpr std::size_t new_alloc_size = 32;
    [[maybe_unused]] auto* p =
        ARENA_REALLOC_ARRAY(&arena, uint8_t, nullptr, 0, new_alloc_size);
    REQUIRE(arena.current == buffer + total_old_alloc_size + new_alloc_size);
  }

  SECTION("Inplace grow")
  {
    static constexpr std::size_t new_alloc_size = 32;
    auto* p2 = ARENA_REALLOC_ARRAY(&arena, uint8_t, p1, second_alloc_size,
                                   new_alloc_size);
    REQUIRE(p2 == p1);
    REQUIRE(arena.current == buffer + first_alloc_size + new_alloc_size);
    REQUIRE(p2[0] == '4');
    REQUIRE(p2[1] == '2');
  }

  SECTION("mismatched alignment")
  {
    static constexpr std::size_t new_alloc_size = 8;
    auto* p2 = static_cast<uint8_t*>(arena_aligned_realloc(
        &arena, p1, 4, second_alloc_size, new_alloc_size));
    REQUIRE(p2 != nullptr);
    REQUIRE(p2 != p1);
    REQUIRE(p2[0] == '4');
    REQUIRE(p2[1] == '2');
    REQUIRE(arena.current ==
            buffer + total_old_alloc_size + 1 + new_alloc_size);
  }

  SECTION("pointer not point to the last allocation")
  {
    static constexpr std::size_t new_alloc_size = 8;
    auto* p2 = ARENA_REALLOC_ARRAY(&arena, uint8_t, p0, first_alloc_size,
                                   new_alloc_size);
    REQUIRE(p2 != nullptr);
    REQUIRE(p2 != p0);
    REQUIRE(p2[0] == 'x');
    REQUIRE(arena.current == buffer + total_old_alloc_size + new_alloc_size);
  }
}
