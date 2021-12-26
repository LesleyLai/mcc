#include <stdio.h>
#include <stdlib.h>

#include "arena.h"
#include "parser.h"
//
// const char* minimum_source = "int main(void)\n"
//                             "{\n"
//                             "  return 42;\n"
//                             "}";
//
// int main(void)
//{
//  const size_t ast_arena_size = 4000000000; // 4 GB virtual memory
//  void* ast_buffer = malloc(ast_arena_size);
//  Arena ast_arena = arena_init(ast_buffer, ast_arena_size);
//  parse(minimum_source, &ast_arena);
//}

const char* source = "section .text\n"
                     "global main\n"
                     "main:\n"
                     "  mov     rax, 2\n"
                     "  ret";

void compile(void)
{
  FILE* file = fopen("file.asm", "w");
  if (!file) {
    perror("Cannot open file");
    exit(1);
  }
  if (fputs(source, file) == EOF) { perror("fputs error"); }
  fclose(file);
}
void assemble(void)
{
#ifdef _WIN32
  system("yasm -fwin64 file.asm -o file.obj");
#else // linux
  system("yasm -felf64 file.asm -o file.o");
#endif
}

void link(void)
{
#ifdef _WIN32
  char buffer[10000];
  snprintf(
      buffer, 10000,
      "link.exe /SUBSYSTEM:CONSOLE /ENTRY:main file.obj /OUT:hello.exe 1>NUL");
  system(buffer);
#else // linux
  system("ld -lc file.o -o hello -I /lib64/ld-linux-x86-64.so.2");

#endif
}

const char* instrs = "  mov eax, 42\n"
                     "  ret 0";

int main(void)
{
  compile();
  assemble();
  link();
}