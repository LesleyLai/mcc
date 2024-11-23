#include <stdio.h>
#include <stdlib.h>

#include "diagnostic.h"
#include "parser.h"
#include "utils/allocators.h"
#include "utils/defer.h"
#include "utils/str.h"

#include <stdarg.h>

static void compile_to_file(FILE* asm_file,
                            const char* src_filename_with_extension,
                            const char* source)
{
  const size_t ast_arena_size = 4000000000; // 4 GB virtual memory
  void* ast_buffer = malloc(ast_arena_size);
  Arena ast_arena = arena_init(ast_buffer, ast_arena_size);

  const ParseResult result = parse(source, &ast_arena);
  const TranslationUnit* tu = result.ast;

  enum { diagnostics_arena_size = 40000 }; // 40 Mb virtual memory
  uint8_t diagnostics_buffer[diagnostics_arena_size];
  Arena diagnostics_arena =
      arena_init(diagnostics_buffer, diagnostics_arena_size);
  PolyAllocator diagnostics_allocator =
      poly_allocator_from_arena(&diagnostics_arena);

  if (result.errors.size > 0) {
    for (size_t i = 0; i < result.errors.size; ++i) {
      StringBuffer output = string_buffer_new(&diagnostics_allocator);
      write_diagnostics(&output, src_filename_with_extension, source,
                        result.errors.data[i]);
      StringView output_view = string_view_from_buffer(&output);
      printf("%*s", (int)output_view.size, output_view.start);
      arena_reset(&diagnostics_arena);
    }
  }

  if (tu == NULL) {
    // Failed to parse the program
    exit(1);
  }

  if (tu->decl_count != 1 || tu->decls[0].body->statement_count != 1 ||
      tu->decls[0].body->statements[0].type != RETURN_STMT) {
    // TODO: Not support yet
    fprintf(stderr, "MCC does not support this kind of program yet");
    exit(1);
  }

  const int return_value =
      tu->decls[0].body->statements[0].ret.expr->const_expr.val;

  // fputs("section .text\n", asm_file);
  fputs(".globl main\n", asm_file);
  fputs("main:\n", asm_file);
  fprintf(asm_file, "  mov     rax, %d\n", return_value);
  fputs("  ret\n", asm_file);
}

void compile(const char* src_filename_with_extension, const char* asm_filename,
             const char* source)
{
  enum { temp_buffer_size = 1000 };
  uint8_t* temp_buffer[temp_buffer_size];
  Arena temp_arena = arena_init(temp_buffer, temp_buffer_size);
  PolyAllocator allocator = poly_allocator_from_arena(&temp_arena);

  const StringView src_filename_sv = string_view_from_c_str(asm_filename);

  StringBuffer asm_filename_with_extension =
      string_buffer_from_view(src_filename_sv, &allocator);
  string_buffer_append(&asm_filename_with_extension,
                       string_view_from_c_str(".asm"));

  FILE* asm_file = fopen(string_buffer_data(&asm_filename_with_extension), "w");
  MCC_DEFER(fclose(asm_file))
  {
    if (!asm_file) {
      fprintf(stderr, "Cannot open asm file %s",
              string_buffer_data(&asm_filename_with_extension));
      exit(1);
    }

    compile_to_file(asm_file, src_filename_with_extension, source);

    if (ferror(asm_file)) { perror("Failed to write asm_file"); }
  }
}

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

char* file_to_allocated_buffer(FILE* file)
{
  fseek(file, 0, SEEK_END);
  const long length = ftell(file);
  if (length < 0) {
    (void)fprintf(stderr, "Error in ftell");
    exit(1);
  }
  const size_t ulength = (size_t)length;

  fseek(file, 0, SEEK_SET);
  char* buffer = malloc(ulength + 1);
  size_t read_res = fread(buffer, 1, ulength,
                          file); // TODO: check result of fread
  if (read_res != (unsigned long)length) {
    (void)fprintf(stderr, "Error during reading file");
    exit(1);
  }
  buffer[ulength] = '\0';
  return buffer;
}

int main(int argc, char* argv[])
{
  if (argc != 2) {
    puts("Usage: mcc <filename>");
    return 1;
  }
  const char* src_filename_with_extension = argv[1];

  FILE* src_file = fopen(src_filename_with_extension, "rb");
  if (!src_file) {
    (void)fprintf(stderr, "Mcc: fatal error: %s: No such file",
                  src_filename_with_extension);
    exit(1);
  }

  char* source = file_to_allocated_buffer(src_file);

  fclose(src_file);

  const char* asm_filename = "file";
  compile(src_filename_with_extension, asm_filename, source);

  free(source);

  const char* obj_filename = asm_filename;
  assemble(asm_filename, obj_filename);

  const char* executable_name = obj_filename;
  link(obj_filename, executable_name);
}
