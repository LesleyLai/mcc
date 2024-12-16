#include "cli_args.h"

#include "utils/prelude.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Option {
  const char* code;
  const char* description;
} Option;

static const Option options[] = {
    {"--help, -h", "prints this help message"},
    {"--lex", "lexing only and then dump the result tokens"},
    {"--parse", "lex and parse, and then dump the result AST"},
    {"--ir", "generate the IR, and then dump the result AST"},
    {"--codegen", "generate the assembly, and then dump the result rather than "
                  "saving to a file"},
    {"-S", "Compile only; do not assemble or link."},
    {"-c", "Compile and assemble, but do not link."}};

void print_usage(FILE* stream)
{
  (void)fputs("Usage: mcc [options] filename...\n", stream);
}

void print_options()
{
  printf("Options:\n");
  for (size_t i = 0; i < MCC_ARRAY_SIZE(options); i++) {
    printf("  %-15s %s\n", options[i].code, options[i].description);
  }
}

CliArgs parse_cli_args(int argc, char** argv)
{
  CliArgs result = {0};

  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
    if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
      print_usage(stdout);
      print_options();
      exit(0);
    } else if (!strcmp(arg, "--lex")) {
      result.stop_after_lexer = true;
    } else if (!strcmp(arg, "--parse")) {
      result.stop_after_parser = true;
    } else if (!strcmp(arg, "-S")) {
      result.compile_only = true;
    } else if (!strcmp(arg, "-c")) {
      result.stop_before_linker = true;
    } else if (!strcmp(arg, "--ir") || !strcmp(arg, "--tacky")) {
      // The --tacky command is used for "Writing a compiler" book's test cases
      result.gen_ir_only = true;
    } else if (!strcmp(arg, "--codegen")) {
      // The --tacky command is used for "Writing a compiler" book's test cases
      result.codegen_only = true;
    } else {
      // TODO: support more than one source file
      result.source_filename = arg;
    }
  }

  if (argc < 2) {
    (void)fputs("mcc: fatal error: no input files\n", stderr);
    print_usage(stderr);
    exit(1);
  }

  return result;
}
