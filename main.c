#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

const char* minimum_source = "int main(void)\n"
                             "{\n"
                             "  return 42;\n"
                             "}";

void compile_source(const char* source)
{
  Lexer lexer = lexer_create(source);
  while (1) {
    Token token = lexer_scan_token(&lexer);
    if (token.type == TOKEN_EOF) break;
    printf("%d %d:%d \"%.*s\"\n", token.type, token.line, token.column,
           (int)(token.src.length), token.src.start);
  }
}

int main(void)
{
  compile_source(minimum_source);
}
