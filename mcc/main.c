#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "utils/allocators.h"
#include "utils/str.h"

static void compile_to_file(FILE* asm_file, const char* source)
{
  const size_t ast_arena_size = 4000000000; // 4 GB virtual memory
  void* ast_buffer = malloc(ast_arena_size);
  Arena ast_arena = arena_init(ast_buffer, ast_arena_size);

  FunctionDecl* main_func = parse(source, &ast_arena);
  if (main_func == NULL) {
    fprintf(stderr, "Failed to parse the program");
    return;
  }

  if (main_func->body->statement_count != 1 ||
      main_func->body->statements[0]->type != RETURN_STMT) {
    // TODO: Not support yet
    fprintf(stderr, "MCC does not support this kind of program yet");
    return;
  }

  const int return_value =
      ((ConstExpr*)((ReturnStmt*)main_func->body->statements[0])->expr)->val;

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

  if (!asm_file) {
    fprintf(stderr, "Cannot open asm file %s",
            string_buffer_data(&asm_filename_with_extension));
    exit(1);
  }

  compile_to_file(asm_file, source);

  if (ferror(asm_file)) { perror("Failed to write asm_file"); }
  fclose(asm_file);
}

void assemble(const char* asm_filename, const char* obj_filename)
{
  enum { buffer_size = 10000 };
  char buffer[buffer_size];

#ifdef _WIN32
  snprintf(buffer, buffer_size, "yasm -fwin64 %s.asm -o %s.obj", asm_filename,
           obj_filename);
#else // linux
  snprintf(buffer, buffer_size, "yasm -felf64 %.*s.asm -o %.*s.o", asm_filename,
           obj_filename);
#endif

  system(buffer);
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
  system(buffer);
}

char* file_to_allocated_buffer(FILE* file)
{
  fseek(file, 0, SEEK_END);
  const long length = ftell(file);

  fseek(file, 0, SEEK_SET);
  char* buffer = malloc(length + 1);
  fread(buffer, 1, length, file); // TODO: check result of fread
  buffer[length] = '\0';
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
    fprintf(stderr, "Cannot open source file %s", src_filename_with_extension);
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
