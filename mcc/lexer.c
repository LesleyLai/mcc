#include <stdbool.h>
#include <string.h>

#include "lexer.h"

Lexer lexer_create(const char* source)
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

static Token lexer_make_token(const Lexer* lexer, TokenType type)
{
  const size_t length = (size_t)(lexer->current - lexer->previous);
  return (Token){
      .src = {.start = lexer->previous, .size = length},
      .type = type,
      .location = {.line = lexer->line,
                   .column = lexer->column - (uint32_t)(length),
                   .offset = (size_t)(lexer->previous - lexer->start)}};
}

static Token lexer_error_token(const Lexer* lexer, StringView error_msg)
{
  return (Token){
      .src = error_msg,
      .type = TOKEN_ERROR,
      .location = {.line = lexer->line,
                   .column = lexer->column,
                   .offset = (size_t)(lexer->previous - lexer->start)}};
}

// consumes the current character and returns it
static char lexer_advance(Lexer* lexer)
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
static char lexer_peek_next(const Lexer* lexer)
{
  if (lexer_is_at_end(lexer)) { return '\0'; }
  return lexer->current[1];
}

static void lexer_skip_c_style_comments(Lexer* lexer)
{
  lexer_advance(lexer);
  lexer_advance(lexer);
  while (true) {
    if (*lexer->current == '*' && lexer_peek_next(lexer) == '/') {
      lexer_advance(lexer);
      lexer_advance(lexer);
      break;
    }
    if (lexer_is_at_end(lexer)) {
      // TODO: Properly handle unclosed C comments
      fprintf(stderr, "mcc fatal error: Unclosed comment");
      exit(1);
    }
    lexer_advance(lexer);
  }
}

static void lexer_skip_cpp_style_comments(Lexer* lexer)
{
  while (*lexer->current != '\n' && !lexer_is_at_end(lexer)) {
    lexer_advance(lexer);
  }
}

static void lexer_skip_whitespace(Lexer* lexer)
{
  for (;;) {
    char c = *lexer->current;
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
    case '\n': lexer_advance(lexer); break;
    case '/': // Comments
    {
      const char next_char = lexer_peek_next(lexer);
      if (next_char == '/') {
        lexer_skip_cpp_style_comments(lexer);
        break;
      } else if (next_char == '*') {
        lexer_skip_c_style_comments(lexer);
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

static Token lexer_scan_number(Lexer* lexer)
{
  while (is_digit(*lexer->current) || can_start_identifier(*lexer->current))
    lexer_advance(lexer);

  // Check integer pattern
  for (const char* cursor = lexer->previous; cursor != lexer->current;
       ++cursor) {
    if (!is_digit(*cursor)) { return lexer_make_token(lexer, TOKEN_ERROR); }
  }

  return lexer_make_token(lexer, TOKEN_INTEGER);
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

static TokenType lexer_get_identifier_type(Lexer* lexer)
{
  switch (lexer->previous[0]) {
  case 'v':
    return check_keyword(lexer, 1, (StringView){.start = "oid", .size = 3},
                         TOKEN_KEYWORD_VOID);
  case 'i':
    return check_keyword(lexer, 1, (StringView){.start = "nt", .size = 2},
                         TOKEN_KEYWORD_INT);
  case 'r':
    return check_keyword(lexer, 1, (StringView){.start = "eturn", .size = 5},
                         TOKEN_KEYWORD_RETURN);
  default: return TOKEN_IDENTIFIER;
  }
}

static Token lexer_scan_identifier(Lexer* lexer)
{
  while (is_digit(*lexer->current) || can_start_identifier(*lexer->current)) {
    lexer_advance(lexer);
  }
  return lexer_make_token(lexer, lexer_get_identifier_type(lexer));
}

Token lexer_scan_token(Lexer* lexer)
{
  lexer_skip_whitespace(lexer);

  lexer->previous = lexer->current;

  if (lexer_is_at_end(lexer)) return lexer_make_token(lexer, TOKEN_EOF);

  const char c = lexer_advance(lexer);
  if (is_digit(c)) return lexer_scan_number(lexer);
  if (can_start_identifier(c)) return lexer_scan_identifier(lexer);

  switch (c) {
  case '(': return lexer_make_token(lexer, TOKEN_LEFT_PAREN);
  case ')': return lexer_make_token(lexer, TOKEN_RIGHT_PAREN);
  case '{': return lexer_make_token(lexer, TOKEN_LEFT_BRACE);
  case '}': return lexer_make_token(lexer, TOKEN_RIGHT_BRACE);
  case ';': return lexer_make_token(lexer, TOKEN_SEMICOLON);
  case '+': return lexer_make_token(lexer, TOKEN_PLUS);
  case '-': return lexer_make_token(lexer, TOKEN_MINUS);
  case '*': return lexer_make_token(lexer, TOKEN_STAR);
  case '/': return lexer_make_token(lexer, TOKEN_SLASH);
  case '%': return lexer_make_token(lexer, TOKEN_PERCENT);
  case '~': return lexer_make_token(lexer, TOKEN_TILDE);
  default: break;
  }

  return lexer_error_token(lexer,
                           string_view_from_c_str("Unexpected character."));
}

Tokens lex(const char* source, Arena* permanent_arena, Arena scratch_arena)
{
  Lexer lexer = lexer_create(source);

  // Accumulate tokens to a temporary linked list, and then flatten it to an
  // array
  typedef struct Node {
    Token token;
    struct Node* previous;
  } Node;

  Node* current = NULL;

  ptrdiff_t token_count = 0;
  while (true) {
    Token token = lexer_scan_token(&lexer);
    Node* previous = current;
    current = ARENA_ALLOC_OBJECT(&scratch_arena, Node);
    *current = (Node){.token = token, .previous = previous};
    ++token_count;

    if (token.type == TOKEN_EOF) { break; }
  }

  Token* tokens =
      ARENA_ALLOC_ARRAY(permanent_arena, Token, (size_t)token_count);
  for (ptrdiff_t i = token_count - 1; i >= 0; --i) {
    tokens[i] = current->token;
    current = current->previous;
  }

  return (Tokens){.begin = tokens, .end = tokens + token_count};
}
