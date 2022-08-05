#include "parser.h"
#include "lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "utils/allocators.h"

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
  fprintf(stderr, "Syntax error at %d:%d: %s\n", token.location.line,
          token.location.column, error_msg);
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

static Expr* parse_expr(Parser* parser);

static Expr* parse_number_literal(Parser* parser)
{
  const SourceLocation first_location = parser->previous.location;
  SourceLocation last_location = first_location;
  last_location.column += (uint32_t)parser->previous.src.length;

  const int val = strtol(parser->previous.src.start, NULL,
                         10); // TODO(llai): replace strtol

  ConstExpr* result = ARENA_ALLOC_OBJECT(parser->ast_arena, ConstExpr);
  *result = (ConstExpr){
      .base = {.type = CONST_EXPR,
               .source_range = (SourceRange){.first = first_location,
                                             .last = last_location}},
      .val = val};
  return (Expr*)result;
}

typedef enum Precedence {
  PREC_NONE = 0,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

static Expr* parse_group(Parser* parser);
static Expr* parse_unary_op(Parser* parser);
static Expr* parse_binary_op(Parser* parser);

typedef Expr* (*ParseFn)(Parser*);

typedef struct ParseRule {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {parse_group, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_PLUS] = {NULL, parse_binary_op, PREC_TERM},
    [TOKEN_MINUS] = {parse_unary_op, parse_binary_op, PREC_TERM},
    [TOKEN_STAR] = {NULL, parse_binary_op, PREC_FACTOR},
    [TOKEN_SLASH] = {NULL, parse_binary_op, PREC_FACTOR},
    [TOKEN_KEYWORD_VOID] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_INT] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_INTEGER] = {parse_number_literal, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

_Static_assert(sizeof(rules) / sizeof(ParseRule) == TOKEN_TYPES_COUNT,
               "Parse rule table should contain all token types");

static ParseRule* get_rule(TokenType operator_type)
{
  return &rules[operator_type];
}

static Expr* parse_precedence(Parser* parser, Precedence precedence)
{
  if (parser->in_panic_mode) { return NULL; }

  const ParseFn prefix_rule = get_rule(parser->previous.type)->prefix;
  if (prefix_rule == NULL) {
    parse_error_at(parser, "Expect valid expression", parser->current);
    return NULL;
  }

  Expr* expr = prefix_rule(parser);

  while (precedence <= get_rule(parser->current.type)->precedence) {
    parse_advance(parser);
    ParseFn infix_rule = get_rule(parser->previous.type)->infix;
    Expr* rhs = infix_rule(parser);
  }

  return expr;
}

static Expr* parse_group(Parser* parser)
{
  Expr* result = parse_expr(parser);
  parse_consume(parser, TOKEN_RIGHT_PAREN, "Expect )");
  return result;
}

static Expr* parse_unary_op(Parser* parser)
{
  TokenType operator_type = parser->previous.type;

  Expr* expr = parse_precedence(parser, PREC_UNARY);

  switch (operator_type) {
  case TOKEN_MINUS: return expr; // TODO: should actually wrap it
  default: return NULL;          // TODO: better error reporting for unreachable
  }
}

static Expr* parse_binary_op(Parser* parser)
{
  const TokenType operator_type = parser->previous.type;
  const ParseRule* rule = get_rule(operator_type);
  parse_precedence(parser, (Precedence)(rule->precedence + 1));

  BinaryOpType binary_op_type;
  switch (operator_type) {
  case TOKEN_PLUS: binary_op_type = BINARY_OP_PLUS; break;
  case TOKEN_MINUS: binary_op_type = BINARY_OP_MINUS; break;
  case TOKEN_STAR: binary_op_type = BINARY_OP_MULT; break;
  case TOKEN_SLASH: binary_op_type = BINARY_OP_DIVIDE; break;
  default: return NULL; // TODO: better error reporting for unreachable
  }

  return NULL;
}

static Expr* parse_expr(Parser* parser)
{
  return parse_precedence(parser, PREC_ASSIGNMENT);
}

static ReturnStmt* parse_return_stmt(Parser* parser, SourceLocation first_loc)
{
  parse_advance(parser);
  Expr* expr = parse_expr(parser);
  if (parser->in_panic_mode) return NULL;

  assert(expr != NULL);

  ReturnStmt* result = ARENA_ALLOC_OBJECT(parser->ast_arena, ReturnStmt);
  *result = (ReturnStmt){
      .base = {.type = RETURN_STMT,
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
      .line = parser->previous.location.line,
      .column = parser->previous.location.column +
                (int)parser->previous.src.length - 1,
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
  const SourceLocation first_loc = parser->current.location;

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

  const SourceLocation first_location = parser->current.location;

  parse_consume(parser, TOKEN_KEYWORD_INT, "Expect keyword int");
  StringView function_name = parse_identifier(parser);

  parse_parameter_list(parser);

  CompoundStmt* body = NULL;

  if (parser->current.type == TOKEN_LEFT_BRACE) { // is definition
    const SourceLocation compound_first_location = parser->current.location;
    parse_advance(parser);
    body = parse_compound_stmt(parser, compound_first_location);
  } else {
    parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  }
  if (parser->in_panic_mode) { return NULL; }

  const SourceLocation last_location =
      (body == NULL)
          ? parser->previous.location
          : body->statements[body->statement_count - 1]->source_range.last;

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