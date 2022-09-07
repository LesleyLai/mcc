#include <ApprovalTests.hpp>
#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>

extern "C" {
#include "parser.h"
#include "utils/allocators.h"
}

#include "ast_printer.hpp"

// This file compiler against bunch of source files and will serialize from
// input to all the important intermediate information for approval test

using std::filesystem::path;

namespace {

[[nodiscard]] auto read_file_to_string(std::string_view path) -> std::string
{
  std::ifstream file{path.data()};

  if (!file.is_open()) {
    fmt::print(stderr, "Cannot open file {}\n", path);
    std::fflush(stdout);
  }

  std::stringstream ss;
  // read file's buffer contents into streams
  ss << file.rdbuf();

  return ss.str();
}

// This class shows how to implement a simple generator for Catch tests
class CFilesInDirectoryGenerator
    : public Catch::Generators::IGenerator<std::filesystem::path> {

  std::filesystem::recursive_directory_iterator itr_;
  std::filesystem::recursive_directory_iterator end_;

public:
  explicit CFilesInDirectoryGenerator(const path& root)
      : itr_{root}, end_{end(itr_)}
  {
    search_next();
  }

  [[nodiscard]] auto get() const -> const std::filesystem::path& override
  {
    return itr_->path();
  }

  [[nodiscard]] auto next() -> bool override
  {
    ++itr_;
    search_next();
    return itr_ != end_;
  }

  void search_next()
  {
    while (itr_ != end_ && itr_->path().extension() != ".c") {
      ++itr_;
    }
  }
};

[[nodiscard]] auto c_files_in_directory(const path& root)
    -> Catch::Generators::GeneratorWrapper<path>
{
  return {std::make_unique<CFilesInDirectoryGenerator>(root)};
}

[[nodiscard]] auto verify_compilation(const std::string& source,
                                      Arena& ast_arena) -> std::string
{
  std::string output;
  fmt::format_to(std::back_inserter(output), "Source:\n============\n");
  fmt::format_to(std::back_inserter(output), "{}\n\n", source);

  const auto* result = parse(source.c_str(), &ast_arena);

  fmt::format_to(std::back_inserter(output), "AST:\n============\n");
  if (result == nullptr) {
    fmt::format_to(std::back_inserter(output), "Fail to parse!");
    return output;
  }

  mcc::print_to_string(output, *result);
  return output;
}

} // anonymous namespace

TEST_CASE("Integration tests")
{
  const size_t ast_arena_size = 40'000'000; // 40 mb virtual memory

  const auto ast_buffer = std::make_unique<std::byte[]>(ast_arena_size);
  Arena ast_arena = arena_init(ast_buffer.get(), ast_arena_size);

  const auto test_src_directory = path{__FILE__}.remove_filename();
  const auto test_data_directory = test_src_directory / "test_data";

  SECTION("Verify all c files")
  {
    auto file_path = GENERATE_COPY(c_files_in_directory(test_data_directory));

    const auto src = read_file_to_string(file_path.string());

    const auto test_relative_dir =
        file_path.lexically_relative(test_src_directory)
            .remove_filename()
            .string();
    const auto test_name = file_path.filename().replace_extension().string();

    [[maybe_unused]] const auto disposer =
        ApprovalTests::Approvals::useApprovalsSubdirectory(test_relative_dir);
    const auto namer = ApprovalTests::TemplatedCustomNamer::create(
        fmt::format("{{TestSourceDirectory}}/{{ApprovalsSubdirectory}}/"
                    "{}.{{ApprovedOrReceived}}.{{FileExtension}}",
                    test_name));

    ApprovalTests::Approvals::verify(verify_compilation(src, ast_arena),
                                     ApprovalTests::Options().withNamer(namer));
    arena_reset(&ast_arena);
  }
}
