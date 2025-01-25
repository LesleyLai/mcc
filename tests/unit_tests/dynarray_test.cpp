#include <catch2/catch_test_macros.hpp>

extern "C" {
#include <mcc/arena.h>
#include <mcc/dynarray.h>
}

#include "arenas.hpp"

template <typename T> struct Vector {
  uint32_t capacity = 0;
  uint32_t length = 0;
  T* data = nullptr;

  void push_back(T x, Arena* arena) { DYNARRAY_PUSH_BACK(this, T, arena, x); }
};

TEST_CASE("DynArray", "[dynarray]")
{
  Arena arena = get_scratch_arena();
  Vector<int> vector;

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

TEST_CASE("Multiple DynArray", "[dynarray]")
{
  Arena arena = get_scratch_arena();
  Vector<uint8_t> vector1;
  Vector<int> vector2;

  REQUIRE(vector1.length == 0);
  REQUIRE(vector2.length == 0);

  vector2.push_back(2, &arena);
  vector1.push_back(1, &arena);
  vector2.push_back(4, &arena);
  vector1.push_back(3, &arena);

  REQUIRE(vector1.length == 2);
  REQUIRE(vector1.data[0] == 1);
  REQUIRE(vector1.data[1] == 3);

  REQUIRE(vector2.length == 2);
  REQUIRE(vector2.data[0] == 2);
  REQUIRE(vector2.data[1] == 4);
}
