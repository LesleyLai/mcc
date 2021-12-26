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

static void parse_expr(Parser* parser, Expr* result)
{
  if (parser->in_panic_mode) { return; }

  if (parser->current.type != TOKEN_INTEGER) {
    parse_error_at(parser, "Expect integer literal", parser->current);
    return;
  }
  char* end;
  result->val =
      strtol(parser->current.src.start, &end, 10); // TODO(llai): replace strtol
  assert(end == parser->current.src.start + parser->current.src.length);
  parse_advance(parser);
}

static void parse_return_stmt(Parser* parser, ReturnStmt* result)
{
  Expr* expr = ARENA_ALLOC_OBJECT(parser->ast_arena, Expr);
  parse_expr(parser, expr);
  if (parser->in_panic_mode) return;

  *result = (ReturnStmt){.expr = expr};
  parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
}

static void parse_stmt(Parser* parser, Stmt* result);

// TODO: Implement proper vector and use it for compound statement
#define MAX_STMT_COUNT_IN_COMPOUND_STMT 16

static void parse_compound_stmt(Parser* parser, CompoundStmt* result)
{
  Stmt* statements = ARENA_ALLOC_ARRAY(parser->ast_arena, Stmt,
                                       MAX_STMT_COUNT_IN_COMPOUND_STMT);
  size_t statement_count = 0;

  bool encounter_error = false;
  while (parser->current.type != TOKEN_RIGHT_BRACE) {
    if (statement_count == MAX_STMT_COUNT_IN_COMPOUND_STMT) {
      parse_error_at(parser,
                     "Too many statement in compound statement to handle",
                     parser->current);
      return;
    }
    parse_stmt(parser, &statements[statement_count]);
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
  *result = (CompoundStmt){.statements = statements,
                           .statement_count = statement_count};
}

static void parse_stmt(Parser* parser, Stmt* result)
{
  switch (parser->current.type) {
  case TOKEN_KEYWORD_RETURN: {
    parse_advance(parser);
    ReturnStmt return_stmt;
    parse_return_stmt(parser, &return_stmt);
    *result = (Stmt){.type = RETURN_STMT, .return_statement = return_stmt};
    break;
  }
  case TOKEN_LEFT_BRACE: {
    parse_advance(parser);
    CompoundStmt compound_stmt;
    parse_compound_stmt(parser, &compound_stmt);
    *result =
        (Stmt){.type = COMPOUND_STMT, .compound_statement = compound_stmt};
    break;
  }
  default: {
    parse_error_at(parser, "Expect statement", parser->current);
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

static void parse_function_decl(Parser* parser, FunctionDecl* result)
{
  parse_consume(parser, TOKEN_KEYWORD_INT, "Expect keyword int");

  StringView function_name = parse_identifier(parser);

  parse_parameter_list(parser);

  CompoundStmt* body = NULL;
  if (parser->current.type == TOKEN_LEFT_BRACE) { // is definition
    parse_advance(parser);
    body = ARENA_ALLOC_OBJECT(parser->ast_arena, CompoundStmt);
    parse_compound_stmt(parser, body);
  } else {
    parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  }

  *result = (FunctionDecl){
      .name = function_name,
      .body = body,
  };
}

FunctionDecl* parse(const char* source, Arena* ast_arena)
{
  Parser parser = {.lexer = lexer_create(source),
                   .ast_arena = ast_arena,
                   .has_error = false,
                   .in_panic_mode = false};

  parse_advance(&parser);

  FunctionDecl* decl = ARENA_ALLOC_OBJECT(parser.ast_arena, FunctionDecl);
  parse_function_decl(&parser, decl);
  parse_consume(&parser, TOKEN_EOF, "Expect end of the file");

  return parser.has_error ? NULL : decl;
}