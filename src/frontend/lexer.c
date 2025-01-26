#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include <mcc/dynarray.h>
#include <mcc/frontend.h>

// The lexer consumes source code and produces tokens lazily

typedef struct Lexer {
  const char* start;
  const char* previous;
  const char* current;
  uint32_t line;
  uint32_t column;
} Lexer;

static Lexer lexer_create(const char* source)
{
  return (Lexer){
      .start = source,
      .previous = source,
      .current = source,
      .line = 1,
      .column = 1,
  };
}

static bool lexer_is_at_end(const Lexer* lexer)
{
  return *lexer->current == '\0';
}

static Token make_token(const Lexer* lexer, TokenType type)
{
  return (Token){
      .type = type,
      .start = u32_from_isize(lexer->previous - lexer->start),
      .size = u32_from_isize(lexer->current - lexer->previous),
  };
}

// consumes the current character and returns it
static char advance(Lexer* lexer)
{
  if (*lexer->current == '\n') {
    ++lexer->line;
    lexer->column = 1;
  } else {
    lexer->column++;
  }

  return *lexer->current++;
}

// Look at the next character
static char peek_next(const Lexer* lexer)
{
  if (lexer_is_at_end(lexer)) { return '\0'; }
  return lexer->current[1];
}

static void skip_c_style_comments(Lexer* lexer)
{
  advance(lexer);
  advance(lexer);
  while (true) {
    if (*lexer->current == '*' && peek_next(lexer) == '/') {
      advance(lexer);
      advance(lexer);
      break;
    }
    if (lexer_is_at_end(lexer)) {
      // TODO: Properly handle unclosed C comments
      fprintf(stderr, "mcc fatal error: Unclosed comment");
      exit(1);
    }
    advance(lexer);
  }
}

static void skip_cpp_style_comments(Lexer* lexer)
{
  while (*lexer->current != '\n' && !lexer_is_at_end(lexer)) {
    advance(lexer);
  }
}

static void skip_whitespace(Lexer* lexer)
{
  for (;;) {
    char c = *lexer->current;
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
    case '\n': advance(lexer); break;
    case '/': // Comments
    {
      const char next_char = peek_next(lexer);
      if (next_char == '/') {
        skip_cpp_style_comments(lexer);
        break;
      } else if (next_char == '*') {
        skip_c_style_comments(lexer);
        break;
      } else {
        return;
      }
    }
    default: return;
    }
  }
}

static bool is_digit(char c)
{
  return c >= '0' && c <= '9';
}

static bool can_start_identifier(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static Token scan_number(Lexer* lexer)
{
  while (is_digit(*lexer->current) || can_start_identifier(*lexer->current))
    advance(lexer);

  // Check integer pattern
  for (const char* cursor = lexer->previous; cursor != lexer->current;
       ++cursor) {
    if (!is_digit(*cursor)) { return make_token(lexer, TOKEN_ERROR); }
  }

  return make_token(lexer, TOKEN_INTEGER);
}

static bool eat_char_if_match(Lexer* lexer, const char expected)
{
  if (lexer_is_at_end(lexer)) { return false; }
  if (*lexer->current != expected) { return false; }

  advance(lexer);
  return true;
}

static TokenType check_keyword(Lexer* lexer, int start_position,
                               StringView rest, TokenType type)
{
  if (lexer->current - lexer->previous == start_position + (int)rest.size &&
      memcmp(lexer->previous + start_position, rest.start, rest.size) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType get_identifier_type(Lexer* lexer)
{
  switch (lexer->previous[0]) {
  case 'b': return check_keyword(lexer, 1, str("reak"), TOKEN_KEYWORD_BREAK);
  case 'c':
    return check_keyword(lexer, 1, str("ontinue"), TOKEN_KEYWORD_CONTINUE);
  case 'd': return check_keyword(lexer, 1, str("o"), TOKEN_KEYWORD_DO);
  case 'e': return check_keyword(lexer, 1, str("lse"), TOKEN_KEYWORD_ELSE);
  case 'f': return check_keyword(lexer, 1, str("or"), TOKEN_KEYWORD_FOR);
  case 'i': {
    if (lexer->current - lexer->previous > 1) {
      switch (lexer->previous[1]) {
      case 'f': return check_keyword(lexer, 2, str(""), TOKEN_KEYWORD_IF);
      case 'n': return check_keyword(lexer, 2, str("t"), TOKEN_KEYWORD_INT);
      default: break;
      }
    }
  } break;
  case 'v': return check_keyword(lexer, 1, str("oid"), TOKEN_KEYWORD_VOID);
  case 'r': return check_keyword(lexer, 1, str("eturn"), TOKEN_KEYWORD_RETURN);
  case 't':
    return check_keyword(lexer, 1, str("ypedef"), TOKEN_KEYWORD_TYPEDEF);
  case 'w': return check_keyword(lexer, 1, str("hile"), TOKEN_KEYWORD_WHILE);
  default: break;
  }
  return TOKEN_IDENTIFIER;
}

static Token scan_identifier(Lexer* lexer)
{
  while (is_digit(*lexer->current) || can_start_identifier(*lexer->current)) {
    advance(lexer);
  }
  return make_token(lexer, get_identifier_type(lexer));
}

static Token scan_symbol(Lexer* lexer)
{
  switch (lexer->previous[0]) {
  case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
  case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
  case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
  case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
  case ';': return make_token(lexer, TOKEN_SEMICOLON);
  case '+': {
    const TokenType token_type = eat_char_if_match(lexer, '+') ? TOKEN_PLUS_PLUS
                                 : eat_char_if_match(lexer, '=')
                                     ? TOKEN_PLUS_EQUAL
                                     : TOKEN_PLUS;
    return make_token(lexer, token_type);
  }
  case '-': {
    const TokenType token_type =
        eat_char_if_match(lexer, '-')   ? TOKEN_MINUS_MINUS
        : eat_char_if_match(lexer, '=') ? TOKEN_MINUS_EQUAL
        : eat_char_if_match(lexer, '>') ? TOKEN_MINUS_GREATER
                                        : TOKEN_MINUS;
    return make_token(lexer, token_type);
  }
  case '*':
    return make_token(lexer, eat_char_if_match(lexer, '=') ? TOKEN_STAR_EQUAL
                                                           : TOKEN_STAR);
  case '/':
    return make_token(lexer, eat_char_if_match(lexer, '=') ? TOKEN_SLASH_EQUAL
                                                           : TOKEN_SLASH);
  case '%':
    return make_token(lexer, eat_char_if_match(lexer, '=') ? TOKEN_PERCENT_EQUAL
                                                           : TOKEN_PERCENT);
  case '~': return make_token(lexer, TOKEN_TILDE);
  case '&': {
    const TokenType token_type =
        eat_char_if_match(lexer, '&')   ? TOKEN_AMPERSAND_AMPERSAND
        : eat_char_if_match(lexer, '=') ? TOKEN_AMPERSAND_EQUAL
                                        : TOKEN_AMPERSAND;
    return make_token(lexer, token_type);
  }
  case '|': {
    const TokenType token_type = eat_char_if_match(lexer, '|') ? TOKEN_BAR_BAR
                                 : eat_char_if_match(lexer, '=')
                                     ? TOKEN_BAR_EQUAL
                                     : TOKEN_BAR;
    return make_token(lexer, token_type);
  }
  case '^':
    return make_token(lexer, eat_char_if_match(lexer, '=') ? TOKEN_CARET_EQUAL
                                                           : TOKEN_CARET);
  case '=':
    return make_token(lexer, eat_char_if_match(lexer, '=') ? TOKEN_EQUAL_EQUAL
                                                           : TOKEN_EQUAL);
  case '!':
    return make_token(lexer, eat_char_if_match(lexer, '=') ? TOKEN_NOT_EQUAL
                                                           : TOKEN_NOT);
  case '<': {
    const TokenType token_type =
        eat_char_if_match(lexer, '<')
            ? (eat_char_if_match(lexer, '=') ? TOKEN_LESS_LESS_EQUAL
                                             : TOKEN_LESS_LESS)
        : eat_char_if_match(lexer, '=') ? TOKEN_LESS_EQUAL
                                        : TOKEN_LESS;
    return make_token(lexer, token_type);
  }
  case '>': {
    const TokenType token_type =
        eat_char_if_match(lexer, '>')
            ? (eat_char_if_match(lexer, '=') ? TOKEN_GREATER_GREATER_EQUAL
                                             : TOKEN_GREATER_GREATER)
        : eat_char_if_match(lexer, '=') ? TOKEN_GREATER_EQUAL
                                        : TOKEN_GREATER;
    return make_token(lexer, token_type);
  }
  case ',': return make_token(lexer, TOKEN_COMMA);
  case '.': return make_token(lexer, TOKEN_DOT);
  case '?': return make_token(lexer, TOKEN_QUESTION);
  case ':': return make_token(lexer, TOKEN_COLON);
  default: break;
  }

  return make_token(lexer, TOKEN_ERROR);
}

static Token scan_token(Lexer* lexer)
{
  skip_whitespace(lexer);

  lexer->previous = lexer->current;

  if (lexer_is_at_end(lexer)) return make_token(lexer, TOKEN_EOF);

  const char c = advance(lexer);
  if (is_digit(c)) return scan_number(lexer);
  if (can_start_identifier(c)) return scan_identifier(lexer);
  return scan_symbol(lexer);
}

struct TokenTypeDynArray {
  size_t length;
  size_t capacity;
  TokenType* data;
};

struct U32DynArray {
  size_t length;
  size_t capacity;
  uint32_t* data;
};

Tokens lex(const char* source, Arena* permanent_arena, Arena scratch_arena)
{
  Lexer lexer = lexer_create(source);

  struct TokenTypeDynArray token_types_dyn_array = {};
  struct U32DynArray token_starts_dyn_array = {};
  struct U32DynArray token_sizes_dyn_array = {};

  while (true) {
    const Token token = scan_token(&lexer);
    DYNARRAY_PUSH_BACK(&token_types_dyn_array, TokenType, &scratch_arena,
                       token.type);
    DYNARRAY_PUSH_BACK(&token_starts_dyn_array, uint32_t, &scratch_arena,
                       token.start);
    DYNARRAY_PUSH_BACK(&token_sizes_dyn_array, uint32_t, &scratch_arena,
                       token.size);
    if (token.type == TOKEN_EOF) { break; }
  }

  const uint32_t token_count = u32_from_usize(token_types_dyn_array.length);
  MCC_ASSERT(token_starts_dyn_array.length == token_count);
  MCC_ASSERT(token_sizes_dyn_array.length == token_count);

  TokenType* token_types =
      ARENA_ALLOC_ARRAY(permanent_arena, TokenType, token_count);
  memcpy(token_types, token_types_dyn_array.data,
         token_count * sizeof(TokenType));

  uint32_t* token_starts =
      ARENA_ALLOC_ARRAY(permanent_arena, uint32_t, token_count);
  memcpy(token_starts, token_starts_dyn_array.data,
         token_count * sizeof(uint32_t));

  uint32_t* token_sizes =
      ARENA_ALLOC_ARRAY(permanent_arena, uint32_t, token_count);
  memcpy(token_sizes, token_sizes_dyn_array.data,
         token_count * sizeof(uint32_t));

  return (Tokens){.token_count = token_count,
                  .token_types = token_types,
                  .token_starts = token_starts,
                  .token_sizes = token_sizes};
}
