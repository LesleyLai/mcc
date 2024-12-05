#ifndef MCC_CLI_ARGS_H
#define MCC_CLI_ARGS_H
#include <stdbool.h>

typedef struct CliArgs {
  const char* source_filename; // Filename of the source file (with extension)
  bool stop_after_lexer;
  bool stop_after_parser;

  bool compile_only;       // Compile only; do not assemble or link
  bool stop_before_linker; // Compile and assemble, do not run linker
} CliArgs;

CliArgs parse_cli_args(int argc, char** argv);

#endif // MCC_CLI_ARGS_H