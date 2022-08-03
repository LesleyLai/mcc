#include "parser.h"
#include "lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "arena.h"

typedef struct Parser {
  Lexer lexer;
  Arena* ast_arena;

  bool has_error;
  bool in_panic_mode;

  Token current;
  Token previous;
} Parser;

static void parse_error_at(Parser* parser, const char* error_msg, Token token)
{
  if (parser->in_panic_mode) return;
  fprintf(stderr, "Syntax error at %d:%d: %s\n", token.line, token.column,
          error_msg);
  parser->has_error = true;
  parser->in_panic_mode = true;
}

static void parse_advance(Parser* parser)
{
  for (;;) {
    parser->previous = parser->current;
    parser->current = lexer_scan_token(&parser->lexer);
    if (parser->current.type != TOKEN_ERROR) break;
    parse_error_at(parser, "Error", parser->current);
  }
}

static void parse_consume(Parser* parser, TokenType type, const char* error_msg)
{
  if (parser->current.type != type) {
    parse_error_at(parser, error_msg, parser->current);
    fprintf(stderr, "Gets %.*s\n", (int)parser->current.src.length,
            parser->current.src.start);
  }

  parse_advance(parser);
}

static Expr* parse_expr(Parser* parser)
{
  if (parser->in_panic_mode) { return NULL; }

  if (parser->current.type != TOKEN_INTEGER) {
    parse_error_at(parser, "Expect integer literal", parser->current);
    return NULL;
  }

  const SourceLocation first_location = {.line = parser->current.line,
                                         .column = parser->current.column};
  SourceLocation last_location = first_location;
  last_location.column += (uint32_t)parser->current.src.length;

  Expr* result = ARENA_ALLOC_OBJECT(parser->ast_arena, Expr);
  result->source_range =
      (SourceRange){.first = first_location, .last = last_location};

  char* end;
  result->val =
      strtol(parser->current.src.start, &end, 10); // TODO(llai): replace strtol
  assert(end == parser->current.src.start + parser->current.src.length);
  parse_advance(parser);
  return result;
}

static ReturnStmt* parse_return_stmt(Parser* parser, SourceLocation first_loc)
{
  Expr* expr = parse_expr(parser);
  if (parser->in_panic_mode) return NULL;

  assert(expr != NULL);

  ReturnStmt* result = ARENA_ALLOC_OBJECT(parser->ast_arena, ReturnStmt);
  *result = (ReturnStmt){
      .base = {.type = RETURN_STMT, // TODO source range
               .source_range = {.first = first_loc,
                                .last = expr->source_range.last}},
      .expr = expr,
  };
  parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  return result;
}

static Stmt* parse_stmt(Parser* parser);

// TODO: Implement proper vector and use it for compound statement
#define MAX_STMT_COUNT_IN_COMPOUND_STMT 16

static CompoundStmt* parse_compound_stmt(Parser* parser,
                                         SourceLocation first_loc)
{
  Stmt** statements = ARENA_ALLOC_ARRAY(parser->ast_arena, Stmt*,
                                        MAX_STMT_COUNT_IN_COMPOUND_STMT);
  size_t statement_count = 0;

  bool encounter_error = false;
  while (parser->current.type != TOKEN_RIGHT_BRACE) {
    if (statement_count == MAX_STMT_COUNT_IN_COMPOUND_STMT) {
      parse_error_at(parser,
                     "Too many statement in compound statement to handle",
                     parser->current);
      return NULL;
    }
    statements[statement_count] = parse_stmt(parser);
    if (parser->in_panic_mode) {
      encounter_error = true;
      break;
    }
    ++statement_count;
  }
  if (encounter_error) {
    while (parser->current.type != TOKEN_RIGHT_BRACE) {
      parse_advance(parser);
    }
  }

  parse_consume(parser, TOKEN_RIGHT_BRACE, "Expect }");

  const SourceLocation last_loc = {
      .line = parser->previous.line,
      .column = parser->previous.column + parser->previous.src.length - 1,
  };

  CompoundStmt* result = ARENA_ALLOC_OBJECT(parser->ast_arena, CompoundStmt);
  *result = (CompoundStmt){
      .base = {.type = COMPOUND_STMT,
               .source_range = {.first = first_loc, .last = last_loc}},
      .statements = statements,
      .statement_count = statement_count};
  return result;
}

static Stmt* parse_stmt(Parser* parser)
{
  const SourceLocation first_loc = {
      .line = parser->current.line,
      .column = parser->current.column,
  };

  switch (parser->current.type) {
  case TOKEN_KEYWORD_RETURN: {
    parse_advance(parser);
    return (Stmt*)parse_return_stmt(parser, first_loc);
  }
  case TOKEN_LEFT_BRACE: {
    parse_advance(parser);
    return (Stmt*)parse_compound_stmt(parser, first_loc);
  }
  default: {
    parse_error_at(parser, "Expect statement", parser->current);
    return NULL;
  }
  }
}

static void parse_parameter_list(Parser* parser)
{
  parse_consume(parser, TOKEN_LEFT_PAREN, "Expect (");
  parse_consume(parser, TOKEN_KEYWORD_VOID, "Expect keyword void");
  parse_consume(parser, TOKEN_RIGHT_PAREN, "Expect )");
}

static StringView parse_identifier(Parser* parser)
{
  if (parser->current.type != TOKEN_IDENTIFIER) {
    parse_error_at(parser, "Expect Identifier", parser->current);
  }
  StringView name = parser->current.src;
  parse_advance(parser);
  return name;
}

static FunctionDecl* parse_function_decl(Parser* parser)
{
  if (parser->in_panic_mode) { return NULL; }

  const SourceLocation first_location = (SourceLocation){
      .line = parser->current.line, .column = parser->current.column};

  parse_consume(parser, TOKEN_KEYWORD_INT, "Expect keyword int");
  StringView function_name = parse_identifier(parser);

  parse_parameter_list(parser);

  CompoundStmt* body = NULL;

  if (parser->current.type == TOKEN_LEFT_BRACE) { // is definition
    const SourceLocation compound_first_location = {
        .line = parser->current.line, .column = parser->current.column};

    parse_advance(parser);
    body = parse_compound_stmt(parser, compound_first_location);
  } else {
    parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  }
  if (parser->in_panic_mode) { return NULL; }

  const SourceLocation last_location =
      body->statements[body->statement_count - 1]->source_range.last;

  FunctionDecl* decl = ARENA_ALLOC_OBJECT(parser->ast_arena, FunctionDecl);
  *decl = (FunctionDecl){
      .source_range = {.first = first_location, .last = last_location},
      .name = function_name,
      .body = body,
  };
  return decl;
}

FunctionDecl* parse(const char* source, Arena* ast_arena)
{
  Parser parser = {.lexer = lexer_create(source),
                   .ast_arena = ast_arena,
                   .has_error = false,
                   .in_panic_mode = false};

  parse_advance(&parser);

  FunctionDecl* decl = parse_function_decl(&parser);
  parse_consume(&parser, TOKEN_EOF, "Expect end of the file");

  return parser.has_error ? NULL : decl;
}