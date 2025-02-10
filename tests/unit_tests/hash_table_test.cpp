#include <catch2/catch_test_macros.hpp>

extern "C" {
#include <mcc/arena.h>
#include <mcc/hash_table.h>
}

#include "arenas.hpp"

TEST_CASE("Hash Map", "[hash_map]")
{
  Arena arena = get_scratch_arena();

  HashMap map = HashMap{.root = nullptr};

  int* value_ptr = ARENA_ALLOC_OBJECT(&arena, int);
  *value_ptr = 42;
  REQUIRE(hashmap_try_insert(&map, str("42"), value_ptr, &arena) == true);

  int* value_ptr2 = ARENA_ALLOC_OBJECT(&arena, int);
  *value_ptr2 = 11;
  REQUIRE(hashmap_try_insert(&map, str("11"), value_ptr2, &arena) == true);

  int* result = (int*)hashmap_lookup(&map, str("2"));
  REQUIRE(result == nullptr);

  result = (int*)hashmap_lookup(&map, str("42"));
  REQUIRE(result != nullptr);
  REQUIRE(*result == 42);

  result = (int*)hashmap_lookup(&map, str("11"));
  REQUIRE(result != nullptr);
  REQUIRE(*result == 11);

  // try insert will not overwrite the entry
  REQUIRE(hashmap_try_insert(&map, str("42"), value_ptr2, &arena) == false);
  result = (int*)hashmap_lookup(&map, str("42"));
  REQUIRE(result != nullptr);
  REQUIRE(*result == 42);
}
