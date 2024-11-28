#include "cli_args.h"

#include <stdio.h>
#include <stdlib.h>

#include "diagnostic.h"
#include "lexer.h"
#include "parser.h"
#include "utils/arena.h"
#include "utils/defer.h"
#include "utils/str.h"

#include <stdarg.h>
#include <string.h>

void assemble(const char* asm_filename, const char* obj_filename)
{
  enum { buffer_size = 10000 };
  char buffer[buffer_size];

  snprintf(buffer, buffer_size,
           "as -c %s.asm -o %s.o -msyntax=intel -mnaked-reg", asm_filename,
           obj_filename);

  if (system(buffer)) {
    fprintf(stderr, "Failed to call the assembler");
    exit(1);
  }
}

void link(const char* obj_filename, const char* executable_name)
{
  enum { buffer_size = 10000 };
  char buffer[buffer_size];

  snprintf(buffer, buffer_size, "gcc %s.o -o %s", obj_filename,
           executable_name);
  if (system(buffer)) {
    fprintf(stderr, "Failed to call the linker");
    exit(1);
  }
}

char* string_from_file(FILE* file, Arena* permanent_arena)
{
  fseek(file, 0, SEEK_END);
  const long length = ftell(file);
  if (length < 0) {
    (void)fprintf(stderr, "Error in ftell");
    exit(1);
  }
  const size_t ulength = (size_t)length;

  fseek(file, 0, SEEK_SET);

  char* buffer = ARENA_ALLOC_ARRAY(permanent_arena, char, ulength + 1);
  size_t read_res = fread(buffer, 1, ulength,
                          file); // TODO: check result of fread
  if (read_res != (unsigned long)length) {
    (void)fprintf(stderr, "Error during reading file");
    exit(1);
  }
  buffer[ulength] = '\0';
  return buffer;
}

void print_parse_diagnostics(ParseErrorsView errors, const char* src_filename,
                             const char* source)
{
  enum { diagnostics_arena_size = 40000 }; // 40 Mb virtual memory
  uint8_t diagnostics_buffer[diagnostics_arena_size];
  Arena diagnostics_arena =
      arena_init(diagnostics_buffer, diagnostics_arena_size);

  if (errors.size > 0) {
    for (size_t i = 0; i < errors.size; ++i) {
      StringBuffer output = string_buffer_new(&diagnostics_arena);
      write_diagnostics(&output, src_filename, source, errors.data[i]);
      StringView output_view = string_view_from_buffer(&output);
      printf("%*s", (int)output_view.size, output_view.start);
      arena_reset(&diagnostics_arena);
    }
  }
}

static void generate_assembly(TranslationUnit* tu)
{
  if (tu->decl_count != 1 || tu->decls[0].body->statement_count != 1 ||
      tu->decls[0].body->statements[0].type != RETURN_STMT) {
    // TODO: Not support yet
    fprintf(stderr, "MCC does not support this kind of program yet");
    exit(1);
  }

  FILE* asm_file = fopen("a.asm", "w");
  MCC_DEFER(fclose(asm_file))
  {
    if (!asm_file) {
      fprintf(stderr, "Cannot open asm file %s", "a.asm");
      exit(1);
    }

    const int return_value =
        tu->decls[0].body->statements[0].ret.expr->const_expr.val;

    fputs(".globl main\n", asm_file);
    fputs("main:\n", asm_file);
    fprintf(asm_file, "  mov     rax, %d\n", return_value);
    fputs("  ret\n", asm_file);

    if (ferror(asm_file)) { perror("Failed to write asm_file"); }
  }
}

int main(int argc, char* argv[])
{
  const size_t permanent_arena_size = 4000000000; // 4 GB virtual memory
  void* permanent_arena_buffer = malloc(permanent_arena_size);
  Arena permanent_arena =
      arena_init(permanent_arena_buffer, permanent_arena_size);

  const size_t scratch_arena_size = 400000000; // 400 MB virtual memory
  void* scratch_arena_buffer = malloc(scratch_arena_size);
  Arena scratch_arena = arena_init(scratch_arena_buffer, scratch_arena_size);

  const CliArgs args = parse_cli_args(argc, argv);

  const char* src_filename_with_extension = args.source_filename;

  FILE* src_file = fopen(src_filename_with_extension, "rb");
  if (!src_file) {
    (void)fprintf(stderr, "Mcc: fatal error: %s: No such file",
                  src_filename_with_extension);
    return 1;
  }

  const char* source = string_from_file(src_file, &permanent_arena);
  fclose(src_file);

  Tokens tokens = lex(source, &permanent_arena, scratch_arena);
  if (args.stop_after_lexer) {
    print_tokens(&tokens);
    exit(0);
  }

  ParseResult parse_result = parse(tokens, &permanent_arena, scratch_arena);
  print_parse_diagnostics(parse_result.errors, src_filename_with_extension,
                          source);

  if (parse_result.ast == NULL) {
    // Failed to parse program
    return 1;
  }

  if (args.stop_after_parser) {
    ast_print_translation_unit(parse_result.ast);
    return 0;
  }

  const char* asm_filename = "a";

  generate_assembly(parse_result.ast);

  const char* obj_filename = asm_filename;
  assemble(asm_filename, obj_filename);

  const char* executable_name = obj_filename;
  link(obj_filename, executable_name);
}
