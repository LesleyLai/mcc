#include <catch2/catch_test_macros.hpp>

extern "C" {
#include <mcc/arena.h>
#include <mcc/hash_table.h>
}

#include "arenas.hpp"

TEST_CASE("Hash Table", "[hash_table]")
{
  Arena arena = get_scratch_arena();

  HashMap table = HashMap{.root = nullptr};

  int* value_ptr = ARENA_ALLOC_OBJECT(&arena, int);
  *value_ptr = 42;
  hashmap_insert(&table, str("42"), value_ptr, &arena);

  int* value_ptr2 = ARENA_ALLOC_OBJECT(&arena, int);
  *value_ptr2 = 11;
  hashmap_insert(&table, str("11"), value_ptr2, &arena);

  void* result = hashmap_lookup(&table, str("2"));
  MCC_ASSERT(result == nullptr);

  int* result2 = (int*)hashmap_lookup(&table, str("42"));
  MCC_ASSERT(result2 != nullptr);
  MCC_ASSERT(*result2 == 42);

  int* result3 = (int*)hashmap_lookup(&table, str("11"));
  MCC_ASSERT(result3 != nullptr);
  MCC_ASSERT(*result3 == 11);
}
