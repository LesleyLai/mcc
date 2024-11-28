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
    {"--lex", "stops afer lexing and dump the result tokens"}};

void print_usage()
{
  puts("Usage: mcc [options] filename...\n");
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
      print_usage();
      print_options();
      exit(0);
    } else if (!strcmp(arg, "--lex")) {
      result.stop_after_lexer = true;
    } else if (!strcmp(arg, "--parse")) {
      result.stop_after_parser = true;
    } else {
      // TODO: support more than one source file
      result.source_filename = arg;
    }
  }

  if (argc < 2) {
    puts("mcc: fatal error: no input files");
    print_usage();
    exit(1);
  }

  return result;
}
