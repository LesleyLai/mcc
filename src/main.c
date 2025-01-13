
#include <stdio.h>
#include <stdlib.h>

#include <mcc/arena.h>
#include <mcc/ast.h>
#include <mcc/cli_args.h>
#include <mcc/diagnostic.h>
#include <mcc/frontend.h>
#include <mcc/ir.h>
#include <mcc/prelude.h>
#include <mcc/str.h>
#include <mcc/x86.h>

#include <stdarg.h>
#include <string.h>

static void assemble(StringView asm_filename, StringView obj_filename)
{
  enum { buffer_size = 10000 };
  char buffer[buffer_size];

  const int print_size = snprintf(
      buffer, buffer_size, "as -c %.*s -o %.*s -msyntax=intel -mnaked-reg",
      (int)asm_filename.size, asm_filename.start, (int)obj_filename.size,
      obj_filename.start);

  MCC_ASSERT_MSG(print_size < buffer_size,
                 "Buffer too small to hold as command");

  if (system(buffer)) {
    (void)fprintf(stderr, "Failed to call the assembler");
    exit(1);
  }
}

static void link(const char* obj_filename, const char* executable_name)
{
  enum { buffer_size = 10000 };
  char buffer[buffer_size];

  const int print_size = snprintf(buffer, buffer_size, "gcc %s -o %s",
                                  obj_filename, executable_name);
  MCC_ASSERT_MSG(print_size < buffer_size,
                 "Buffer too small to hold linker command");

  if (system(buffer)) {
    (void)fprintf(stderr, "Failed to call the linker");
    exit(1);
  }
}

static char* string_from_file(FILE* file, Arena* permanent_arena)
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

static void print_parse_diagnostics(ParseErrorsView errors,
                                    const char* src_filename,
                                    const char* source)
{
  enum { diagnostics_arena_size = 40000 }; // 40 Mb virtual memory
  uint8_t diagnostics_buffer[diagnostics_arena_size];
  Arena diagnostics_arena =
      arena_init(diagnostics_buffer, diagnostics_arena_size);

  for (size_t i = 0; i < errors.length; ++i) {
    StringBuffer output = string_buffer_new(&diagnostics_arena);
    write_diagnostics(&output, src_filename, source, errors.data[i]);
    StringView output_view = string_view_from_buffer(&output);
    fprintf(stderr, "%*s\n", (int)output_view.size, output_view.start);
    arena_reset(&diagnostics_arena);
  }
}

static StringBuffer replace_extension(const char* filename, const char* ext,
                                      Arena* permanent_arena)
{
  const char* dot = strrchr(filename, '.');

  StringView name_without_ext =
      (StringView){filename, (size_t)(dot - filename)};

  StringBuffer output = string_buffer_new(permanent_arena);
  string_buffer_append(&output, name_without_ext);
  string_buffer_append(&output, string_view_from_c_str(ext));

  return output;
}

static void save_x86_asm_file(const char* filename, const X86Program* program)
{
  FILE* asm_file = fopen(filename, "w");
  if (!asm_file) {
    fprintf(stderr, "Cannot open asm file %s", "a.asm");
    exit(1);
  }
  MCC_DEFER(fclose(asm_file))
  {
    x86_dump_assembly(program, asm_file);
    if (ferror(asm_file)) { perror("Failed to write asm_file"); }
  }
}

static void preprocess(const char* src_filename,
                       const char* preprocessed_filename)
{
  enum { buffer_size = 10000 };
  char buffer[buffer_size];

  const int print_size = snprintf(buffer, buffer_size, "gcc -E -P %s -o %s",
                                  src_filename, preprocessed_filename);
  MCC_ASSERT_MSG(print_size < buffer_size,
                 "Buffer too small to hold preprocessing command");
  if (system(buffer)) {
    (void)fprintf(stderr, "Failed to call the preprocessor");
    exit(1);
  }
}

int main(int argc, char* argv[])
{
  // 4 GB virtual memory
  Arena permanent_arena = arena_from_virtual_mem(4000000000);

  // 40 MB virtual memory
  Arena scratch_arena = arena_from_virtual_mem(40000000);

  const CliArgs args = parse_cli_args(argc, argv);

  const char* src_filename = args.source_filename;

  const StringBuffer preprocessed_filename =
      (replace_extension(src_filename, ".i", &permanent_arena));
  const char* preprocessed_filename_cstr =
      string_buffer_c_str(&preprocessed_filename);
  preprocess(src_filename, preprocessed_filename_cstr);

  FILE* preprocessed_file = fopen(preprocessed_filename_cstr, "rb");
  if (!preprocessed_file) {
    (void)fprintf(stderr, "Mcc: fatal error: %s: No such file",
                  preprocessed_filename_cstr);
    return 1;
  }

  const char* source = string_from_file(preprocessed_file, &permanent_arena);
  fclose(preprocessed_file);

  Tokens tokens = lex(source, &permanent_arena, scratch_arena);
  if (args.stop_after_lexer) {
    const LineNumTable* line_num_table =
        get_line_num_table(src_filename, string_view_from_c_str(source),
                           &permanent_arena, scratch_arena);

    print_tokens(source, &tokens, line_num_table);

    bool has_error = false;
    for (uint32_t i = 0; i < tokens.token_count; ++i) {
      if (tokens.token_types[i] == TOKEN_ERROR) { has_error = true; }
    }
    exit(has_error ? 1 : 0);
  }

  ParseResult parse_result =
      parse(src_filename, source, tokens, &permanent_arena, scratch_arena);
  print_parse_diagnostics(parse_result.errors, src_filename, source);

  if (parse_result.ast == NULL) {
    // Failed to parse program
    return 1;
  }

  if (args.stop_after_parser) {
    ast_print_translation_unit(parse_result.ast);
    return 0;
  }

  IRProgram* ir =
      ir_generate(parse_result.ast, &permanent_arena, scratch_arena);

  if (args.gen_ir_only) {
    print_ir(ir);
    return 0;
  }

  const X86Program x86_program =
      x86_generate_assembly(ir, &permanent_arena, scratch_arena);
  if (args.codegen_only) {
    x86_dump_assembly(&x86_program, stdout);
    return 0;
  }

  const StringBuffer asm_filename =
      replace_extension(src_filename, ".s", &permanent_arena);
  save_x86_asm_file(string_buffer_c_str(&asm_filename), &x86_program);

  if (args.compile_only) { return 0; }

  const StringBuffer obj_filename =
      replace_extension(src_filename, ".o", &permanent_arena);
  assemble(string_view_from_buffer(&asm_filename),
           string_view_from_buffer(&obj_filename));

  if (args.stop_before_linker) { return 0; }

  const StringBuffer executable_name =
      replace_extension(src_filename, "", &permanent_arena);
  link(string_buffer_c_str(&obj_filename),
       string_buffer_c_str(&executable_name));
}
