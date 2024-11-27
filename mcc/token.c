//
// Created by lesley on 11/27/24.
//

#include "token.h"

#include <stdio.h>

static const char* token_type_string(TokenType type)
{
  switch (type) {
  case TOKEN_LEFT_PAREN: return "(";
  case TOKEN_RIGHT_PAREN: return ")";
  case TOKEN_LEFT_BRACE: return "{";
  case TOKEN_RIGHT_BRACE: return "}";
  case TOKEN_SEMICOLON: return ";";
  case TOKEN_PLUS: return "+";
  case TOKEN_MINUS: return "-";
  case TOKEN_STAR: return "*";
  case TOKEN_SLASH: return "/";
  case TOKEN_KEYWORD_VOID: return "void";
  case TOKEN_KEYWORD_INT: return "int";
  case TOKEN_KEYWORD_RETURN: return "return";
  case TOKEN_IDENTIFIER: return "identifier";
  case TOKEN_INTEGER: return "integer";
  case TOKEN_ERROR: return "error";
  case TOKEN_EOF: return "EOF";
  case TOKEN_TYPES_COUNT: break;
  }
  return "unknown";
}

void print_tokens(Tokens* tokens)
{
  for (Token* token_itr = tokens->begin; token_itr != tokens->end;
       ++token_itr) {
    Token token = *token_itr;
    printf("%-10s src=\"%.*s\"%*s line=%-2i column=%-2i offset=%li\n",
           token_type_string(token.type), (int)token.src.size, token.src.start,
           10 - (int)token.src.size, "", token.location.line,
           token.location.column, token.location.offset);
  }
}