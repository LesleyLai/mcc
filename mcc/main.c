#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "utils/allocators.h"
#include "utils/defer.h"
#include "utils/str.h"

#include <stdarg.h>

static void compile_to_file(FILE* asm_file, const char* source)
{
  const size_t ast_arena_size = 4000000000; // 4 GB virtual memory
  void* ast_buffer = malloc(ast_arena_size);
  Arena ast_arena = arena_init(ast_buffer, ast_arena_size);

  const TranslationUnit* tu = parse(source, &ast_arena);
  if (tu == NULL) {
    fprintf(stderr, "Failed to parse the program");
    return;
  }

  if (tu->decl_count != 1 || tu->decls[0].body->statement_count != 1 ||
      tu->decls[0].body->statements[0].type != RETURN_STMT) {
    // TODO: Not support yet
    fprintf(stderr, "MCC does not support this kind of program yet");
    return;
  }

  const int return_value =
      tu->decls[0].body->statements[0].ret.expr->const_expr.val;

  /*
    StringBuffer buffer = string_buffer_new(NULL);
    string_buffer_append(&buffer, string_view_from_c_str("section .text\n"));
    string_buffer_append(&buffer, string_view_from_c_str("global main\n"
                                                     "main:\n"));
   */

  fputs("section .text\n", asm_file);
  fputs("global main\n", asm_file);
  fputs("main:\n", asm_file);
  fprintf(asm_file, "  mov     rax, %d\n", return_value);
  fputs("  ret", asm_file);
}

void compile(const char* asm_filename, const char* source)
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

    compile_to_file(asm_file, source);

    if (ferror(asm_file)) { perror("Failed to write asm_file"); }
  }
}

void assemble(const char* asm_filename, const char* obj_filename)
{
  enum { buffer_size = 10000 };
  char buffer[buffer_size];

#ifdef _WIN32
  snprintf(buffer, buffer_size, "yasm -fwin64 %s.asm -o %s.obj", asm_filename,
           obj_filename);
#else // linux
  snprintf(buffer, buffer_size, "yasm -felf64 %s.asm -o %s.o", asm_filename,
           obj_filename);
#endif

  if (system(buffer)) {
    fprintf(stderr, "Failed to call the assembler");
    exit(1);
  }
}

void link(const char* obj_filename, const char* executable_name)
{
  enum { buffer_size = 10000 };
  char buffer[buffer_size];
#ifdef _WIN32
  snprintf(buffer, buffer_size,
           "link.exe /SUBSYSTEM:CONSOLE /ENTRY:main %s.obj /OUT:%s.exe 1>NUL",
           obj_filename, executable_name);
#else // linux
  snprintf(buffer, buffer_size,
           "ld -lc %s.o -o %s -I /lib64/ld-linux-x86-64.so.2", obj_filename,
           executable_name);
#endif
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
    fprintf(stderr, "Error in ftell");
    exit(1);
  }
  const size_t ulength = (size_t)length;

  fseek(file, 0, SEEK_SET);
  char* buffer = malloc(ulength + 1);
  size_t read_res = fread(buffer, 1, ulength,
                          file); // TODO: check result of fread
  if (read_res != (unsigned long)length) {
    fprintf(stderr, "Error during reading file");
    exit(1);
  }
  buffer[ulength] = '\0';
  return buffer;
}

int main(int argc, char* argv[])
{
  if (argc != 2) {
    puts("Usage: mcc <asm_filename>");
    return 1;
  }
  const char* src_filename_with_extension = argv[1];

  FILE* src_file = fopen(src_filename_with_extension, "rb");
  if (!src_file) {
    fprintf(stderr, "Mcc: fatal error: %s: No such file",
            src_filename_with_extension);
    exit(1);
  }

  char* source = file_to_allocated_buffer(src_file);

  const char* asm_filename = "file";
  compile(asm_filename, source);

  free(source);

  const char* obj_filename = asm_filename;
  assemble(asm_filename, obj_filename);

  const char* executable_name = obj_filename;
  link(obj_filename, executable_name);
}
