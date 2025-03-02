#include <stdio.h>
#include <stdlib.h>

#include <mcc/arena.h>
#include <mcc/ast.h>
#include <mcc/cli_args.h>
#include <mcc/diagnostic.h>
#include <mcc/frontend.h>
#include <mcc/ir.h>
#include <mcc/prelude.h>
#include <mcc/sema.h>
#include <mcc/str.h>
#include <mcc/type.h>
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

static StringBuffer replace_extension(const char* filename, const char* ext,
                                      Arena* permanent_arena)
{
  const char* dot = strrchr(filename, '.');

  StringView name_without_ext =
      (StringView){filename, (size_t)(dot - filename)};

  StringBuffer output = string_buffer_new(permanent_arena);
  string_buffer_append(&output, name_without_ext);
  string_buffer_append(&output, str(ext));

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

  const char* src_start = string_from_file(preprocessed_file, &permanent_arena);
  StringView source_str = str(src_start);
  fclose(preprocessed_file);

  Tokens tokens = lex(src_start, &permanent_arena, scratch_arena);
  if (args.stop_after_lexer) {
    const LineNumTable* line_num_table = get_line_num_table(
        src_filename, source_str, &permanent_arena, scratch_arena);

    print_tokens(src_start, &tokens, line_num_table);

    bool has_error = false;
    for (uint32_t i = 0; i < tokens.token_count; ++i) {
      if (tokens.token_types[i] == TOKEN_ERROR) { has_error = true; }
    }
    exit(has_error ? 1 : 0);
  }

  ParseResult parse_result =
      parse(src_start, tokens, &permanent_arena, scratch_arena);
  const DiagnosticsContext diagnostics_context = create_diagnostic_context(
      src_filename, source_str, &permanent_arena, scratch_arena);
  print_diagnostics(parse_result.errors, &diagnostics_context);

  if (parse_result.ast == NULL) {
    // Failed to parse program
    return 1;
  }
  TranslationUnit* tu = parse_result.ast;
  if (args.stop_after_parser) {
    StringView ast_str = string_from_ast(tu, &permanent_arena);
    printf("%.*s\n", (int)ast_str.size, ast_str.start);
    return 0;
  }

  ErrorsView type_errors = type_check(tu, &permanent_arena);
  if (type_errors.length != 0) {
    print_diagnostics(type_errors, &diagnostics_context);
    return 1;
  }
  if (args.stop_after_semantic_analysis) { return 0; }

  IRGenerationResult ir_gen_result =
      ir_generate(tu, &permanent_arena, scratch_arena);

  if (ir_gen_result.program == NULL) {
    // Failed to generate IR
    print_diagnostics(ir_gen_result.errors, &diagnostics_context);
    return 1;
  }

  IRProgram* ir = ir_gen_result.program;

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
  assemble(str_from_buffer(&asm_filename), str_from_buffer(&obj_filename));

  if (args.stop_before_linker) { return 0; }

  const StringBuffer executable_name =
      replace_extension(src_filename, "", &permanent_arena);
  link(string_buffer_c_str(&obj_filename),
       string_buffer_c_str(&executable_name));
}
