#include <mcc/token.h>

#include <stdio.h>

static const char* token_type_string(TokenType type)
{
  switch (type) {
  case TOKEN_INVALID: MCC_UNREACHABLE();
  case TOKEN_LEFT_PAREN: return "(";
  case TOKEN_RIGHT_PAREN: return ")";
  case TOKEN_LEFT_BRACE: return "{";
  case TOKEN_RIGHT_BRACE: return "}";
  case TOKEN_LEFT_BRACKET: return "[";
  case TOKEN_RIGHT_BRACKET: return "]";
  case TOKEN_SEMICOLON: return ";";
  case TOKEN_PLUS: return "+";
  case TOKEN_PLUS_PLUS: return "++";
  case TOKEN_PLUS_EQUAL: return "+=";
  case TOKEN_MINUS: return "-";
  case TOKEN_MINUS_MINUS: return "--";
  case TOKEN_MINUS_EQUAL: return "-=";
  case TOKEN_MINUS_GREATER: return "->";
  case TOKEN_STAR: return "*";
  case TOKEN_STAR_EQUAL: return "*=";
  case TOKEN_SLASH: return "/";
  case TOKEN_SLASH_EQUAL: return "/=";
  case TOKEN_PERCENT: return "%";
  case TOKEN_PERCENT_EQUAL: return "%=";
  case TOKEN_TILDE: return "~";
  case TOKEN_AMPERSAND: return "&";
  case TOKEN_AMPERSAND_AMPERSAND: return "&&";
  case TOKEN_AMPERSAND_EQUAL: return "&=";
  case TOKEN_BAR: return "|";
  case TOKEN_BAR_BAR: return "||";
  case TOKEN_BAR_EQUAL: return "|=";
  case TOKEN_CARET: return "^";
  case TOKEN_CARET_EQUAL: return "^=";
  case TOKEN_EQUAL: return "=";
  case TOKEN_EQUAL_EQUAL: return "==";
  case TOKEN_NOT: return "!";
  case TOKEN_NOT_EQUAL: return "!=";
  case TOKEN_LESS: return "<";
  case TOKEN_LESS_LESS: return "<<";
  case TOKEN_LESS_LESS_EQUAL: return "<<=";
  case TOKEN_LESS_EQUAL: return "<=";
  case TOKEN_GREATER: return ">";
  case TOKEN_GREATER_GREATER: return ">>";
  case TOKEN_GREATER_GREATER_EQUAL: return ">>=";
  case TOKEN_GREATER_EQUAL: return ">=";
  case TOKEN_COMMA: return ",";
  case TOKEN_DOT: return ".";
  case TOKEN_QUESTION: return "?";
  case TOKEN_COLON: return ":";
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

    int src_padding_size = 10 - (int)token.src.size;
    if (src_padding_size < 0) src_padding_size = 0;

    printf("%-10s src=\"%.*s\"%*s line=%-2i column=%-2i offset=%li\n",
           token_type_string(token.type), (int)token.src.size, token.src.start,
           src_padding_size, "", token.location.line, token.location.column,
           token.location.offset);
  }
}