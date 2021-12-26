#include <catch2/catch.hpp>
#include <fmt/format.h>

extern "C" {
#include "arena.h"
#include "parser.h"
}

#include "ast_printer.hpp"

TEST_CASE("Parser")
{
  constexpr const char* minimum_source = "int main(void)\n"
                                         "{\n"
                                         "  return 42;\n"
                                         "}";

  const size_t ast_arena_size = 4'000'000'000; // 4 GB virtual memory
  void* ast_buffer = malloc(ast_arena_size);
  Arena ast_arena = arena_init(ast_buffer, ast_arena_size);

  auto* result = parse(minimum_source, &ast_arena);
  fmt::print("\n\n");
  if (result == nullptr) {
    fmt::print("Fail to parse!\n");
  } else {
    std::string s;
    mcc::print_to_string(s, *result);
    fmt::print("{}\n", s);
  }
}