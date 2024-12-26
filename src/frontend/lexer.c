#include <stdbool.h>
#include <string.h>

#include <mcc/parser.h>

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
  const size_t length = (size_t)(lexer->current - lexer->previous);
  return (Token){
      .src = {.start = lexer->previous, .size = length},
      .type = type,
      .location = {.line = lexer->line,
                   .column = lexer->column - (uint32_t)(length),
                   .offset = (size_t)(lexer->previous - lexer->start)}};
}

static Token error_token(const Lexer* lexer, StringView error_msg)
{
  return (Token){
      .src = error_msg,
      .type = TOKEN_ERROR,
      .location = {.line = lexer->line,
                   .column = lexer->column,
                   .offset = (size_t)(lexer->previous - lexer->start)}};
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

static bool match(Lexer* lexer, const char expected)
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

static Token scan_identifier(Lexer* lexer)
{
  while (is_digit(*lexer->current) || can_start_identifier(*lexer->current)) {
    advance(lexer);
  }
  return make_token(lexer, get_identifier_type(lexer));
}

static Token scan_token(Lexer* lexer)
{
  skip_whitespace(lexer);

  lexer->previous = lexer->current;

  if (lexer_is_at_end(lexer)) return make_token(lexer, TOKEN_EOF);

  const char c = advance(lexer);
  if (is_digit(c)) return scan_number(lexer);
  if (can_start_identifier(c)) return scan_identifier(lexer);

  switch (c) {
  case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
  case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
  case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
  case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
  case ';': return make_token(lexer, TOKEN_SEMICOLON);
  case '+': {
    const TokenType token_type = match(lexer, '+')   ? TOKEN_PLUS_PLUS
                                 : match(lexer, '=') ? TOKEN_PLUS_EQUAL
                                                     : TOKEN_PLUS;
    return make_token(lexer, token_type);
  }
  case '-': {
    const TokenType token_type = match(lexer, '-')   ? TOKEN_MINUS_MINUS
                                 : match(lexer, '=') ? TOKEN_MINUS_EQUAL
                                 : match(lexer, '>') ? TOKEN_MINUS_GREATER
                                                     : TOKEN_MINUS;
    return make_token(lexer, token_type);
  }
  case '*':
    return make_token(lexer, match(lexer, '=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
  case '/':
    return make_token(lexer,
                      match(lexer, '=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
  case '%':
    return make_token(lexer,
                      match(lexer, '=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
  case '~': return make_token(lexer, TOKEN_TILDE);
  case '&': {
    const TokenType token_type = match(lexer, '&')   ? TOKEN_AMPERSAND_AMPERSAND
                                 : match(lexer, '=') ? TOKEN_AMPERSAND_EQUAL
                                                     : TOKEN_AMPERSAND;
    return make_token(lexer, token_type);
  }
  case '|': {
    const TokenType token_type = match(lexer, '|')   ? TOKEN_BAR_BAR
                                 : match(lexer, '=') ? TOKEN_BAR_EQUAL
                                                     : TOKEN_BAR;
    return make_token(lexer, token_type);
  }
  case '^':
    return make_token(lexer,
                      match(lexer, '=') ? TOKEN_CARET_EQUAL : TOKEN_CARET);
  case '=':
    return make_token(lexer,
                      match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '!':
    return make_token(lexer, match(lexer, '=') ? TOKEN_NOT_EQUAL : TOKEN_NOT);
  case '<': {
    const TokenType token_type =
        match(lexer, '<')
            ? (match(lexer, '=') ? TOKEN_LESS_LESS_EQUAL : TOKEN_LESS_LESS)
        : match(lexer, '=') ? TOKEN_LESS_EQUAL
                            : TOKEN_LESS;
    return make_token(lexer, token_type);
  }
  case '>': {
    const TokenType token_type =
        match(lexer, '>')   ? (match(lexer, '=') ? TOKEN_GREATER_GREATER_EQUAL
                                                 : TOKEN_GREATER_GREATER)
        : match(lexer, '=') ? TOKEN_GREATER_EQUAL
                            : TOKEN_GREATER;
    return make_token(lexer, token_type);
  }
  case ',': return make_token(lexer, TOKEN_COMMA);
  case '.': return make_token(lexer, TOKEN_DOT);
  case '?': return make_token(lexer, TOKEN_QUESTION);
  case ':': return make_token(lexer, TOKEN_COLON);
  default: break;
  }

  return error_token(lexer, string_view_from_c_str("Unexpected character."));
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
    Token token = scan_token(&lexer);
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
