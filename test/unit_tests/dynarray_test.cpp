#include <catch2/catch_test_macros.hpp>

extern "C" {
#include <mcc/arena.h>
#include <mcc/dynarray.h>
}

struct IntVector {
  uint32_t capacity = 0;
  uint32_t length = 0;
  int* data = nullptr;

  void push_back(int x, Arena* arena)
  {
    DYNARRAY_PUSH_BACK(this, int, arena, x);
  }
};

TEST_CASE("DynArray", "[dynarray]")
{
  Arena arena = arena_from_virtual_mem(1024);
  IntVector vector;

  REQUIRE(vector.length == 0);
  REQUIRE(vector.capacity == 0);
  REQUIRE(vector.data == nullptr);

  vector.push_back(101, &arena);
  REQUIRE(vector.length == 1);
  REQUIRE(vector.capacity == 16);
  REQUIRE(vector.data[0] == 101);

  vector.push_back(42, &arena);
  REQUIRE(vector.length == 2);
  REQUIRE(vector.capacity == 16);
  REQUIRE(vector.data[0] == 101);
  REQUIRE(vector.data[1] == 42);

  for (int i = 0; i < 16; ++i) {
    vector.push_back(i, &arena);
  }

  REQUIRE(vector.length == 18);
  REQUIRE(vector.capacity == 32);
  REQUIRE(vector.data[0] == 101);
  REQUIRE(vector.data[1] == 42);
  for (int i = 0; i < 16; ++i) {
    REQUIRE(vector.data[i + 2] == i);
  }
}
