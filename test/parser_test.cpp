#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <memory>

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

  const auto ast_buffer = std::make_unique<std::byte[]>(ast_arena_size);
  Arena ast_arena = arena_init(ast_buffer.get(), ast_arena_size);

  std::puts("Source:\n============");
  std::puts(minimum_source);

  const auto* result = parse(minimum_source, &ast_arena);
  std::puts("\n\nAST:\n============");
  if (result == nullptr) {
    std::puts("Fail to parse!");
  } else {
    std::string s;
    mcc::print_to_string(s, *result);
    fmt::print("{}\n", s);
  }
}