#include "parser.h"

#include "utils/format.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_ERROR_COUNT 16 // TODO: use vector

typedef struct Parser {
  Arena* permanent_arena;
  Arena scratch_arena;

  Tokens tokens;
  int current_token_index;

  bool has_error;
  bool in_panic_mode;

  ParseErrorsView errors;
} Parser;

static SourceRange token_source_range(Token token)
{
  return (SourceRange){
      .begin = token.location,
      .end = {.line = token.location.line,
              .column = token.location.column + (uint32_t)token.src.size,
              .offset = token.location.offset + token.src.size}};
}

static SourceRange source_range_union(SourceRange lhs, SourceRange rhs)
{
  return (SourceRange){
      .begin = (lhs.begin.offset < rhs.begin.offset) ? lhs.begin : rhs.begin,
      .end = (lhs.end.offset > rhs.end.offset) ? lhs.end : rhs.end};
}

static void parse_error_at(Parser* parser, StringView error_msg, Token token)
{
  if (parser->in_panic_mode) return;

  // TODO: proper error handling
  MCC_ASSERT_MSG(parser->errors.size < MAX_ERROR_COUNT,
                 "Too many data for mcc to handle");

  if (parser->errors.data == NULL) {
    parser->errors.data =
        ARENA_ALLOC_ARRAY(parser->permanent_arena, ParseError, MAX_ERROR_COUNT);
  }

  parser->errors.data[parser->errors.size++] =
      (ParseError){.msg = error_msg, .range = token_source_range(token)};

  parser->has_error = true;
  parser->in_panic_mode = true;
}

static void assert_in_range(Parser* parser, int token_index)
{
  if (token_index < 0) {
    (void)fprintf(stderr, "Negative token index %i!\n", token_index);
    exit(1);
  }

  if (parser->tokens.begin + token_index >= parser->tokens.end) {
    (void)fprintf(stderr, "Token index out of bound: %i (total %li)!\n",
                  token_index, parser->tokens.end - parser->tokens.begin);
    exit(1);
  }
}

// gets the current token
static Token parser_current_token(Parser* parser)
{
  assert_in_range(parser, parser->current_token_index);
  return parser->tokens.begin[parser->current_token_index];
}

// gets the current token
static Token parser_previous_token(Parser* parser)
{
  int previous_token_index = parser->current_token_index - 1;
  assert_in_range(parser, previous_token_index);
  return parser->tokens.begin[previous_token_index];
}

// whether we are at the end
static bool is_at_end(Parser* parser)
{
  return parser_current_token(parser).type == TOKEN_EOF;
}

// Advance tokens by one
// Also skip any error tokens
static void parse_advance(Parser* parser)
{
  if (is_at_end(parser)) { return; }

  for (;;) {
    parser->current_token_index++;
    Token current = parser_current_token(parser);
    if (current.type != TOKEN_ERROR) break;
    parse_error_at(parser, current.src, current);
  }
}

// Consume the current token. If the token doesn't have specified type, generate
// an error.
static void parse_consume(Parser* parser, TokenType type, const char* error_msg)
{
  const Token current = parser_current_token(parser);

  if (current.type != type) {
    parse_error_at(parser, string_view_from_c_str(error_msg), current);
  }

  parse_advance(parser);
}

static Expr* parse_number_literal(Parser* parser)
{
  const Token token = parser_previous_token(parser);

  char* end_ptr;
  const int val =
      (int)strtol(token.src.start, &end_ptr, 10); // TODO(llai): replace strtol

  MCC_ASSERT_MSG(end_ptr == token.src.start + token.src.size,
                 "Not used all characters for numbers");

  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *result = (Expr){.type = EXPR_TYPE_CONST,
                   .source_range = token_source_range(token),
                   .const_expr = (struct ConstExpr){.val = val}};
  return result;
}

/* =============================================================================
 * Pratt Parsing for expressions
 * =============================================================================
 */
typedef enum Precedence {
  PREC_NONE = 0,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + - ! ~
  PREC_FACTOR,     // * / %
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

static Expr* parse_expr(Parser* parser);
static Expr* parse_group(Parser* parser);
static Expr* parse_unary_op(Parser* parser);
static Expr* parse_binary_op_left_associative(Parser* parser, Expr* lhs_expr);

typedef Expr* (*PrefixParseFn)(Parser*);
typedef Expr* (*InfixParseFn)(Parser*, Expr*);

typedef struct ParseRule {
  PrefixParseFn prefix;
  InfixParseFn infix;
  Precedence precedence;
} ParseRule;

static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {parse_group, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_PLUS] = {NULL, parse_binary_op_left_associative, PREC_TERM},
    [TOKEN_MINUS] = {parse_unary_op, parse_binary_op_left_associative,
                     PREC_TERM},
    [TOKEN_STAR] = {NULL, parse_binary_op_left_associative, PREC_FACTOR},
    [TOKEN_SLASH] = {NULL, parse_binary_op_left_associative, PREC_FACTOR},
    [TOKEN_PERCENT] = {NULL, parse_binary_op_left_associative, PREC_FACTOR},
    [TOKEN_TILDE] = {parse_unary_op, NULL, PREC_TERM},
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
  parse_advance(parser);

  const PrefixParseFn prefix_rule =
      get_rule(parser_previous_token(parser).type)->prefix;
  if (prefix_rule == NULL) {
    parse_error_at(parser, string_view_from_c_str("Expect valid expression"),
                   parser_previous_token(parser));
    return NULL;
  }

  Expr* expr = prefix_rule(parser);

  while (precedence <=
         get_rule(parser_current_token(parser).type)->precedence) {
    parse_advance(parser);
    InfixParseFn infix_rule =
        get_rule(parser_previous_token(parser).type)->infix;
    expr = infix_rule(parser, expr);
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
  Token operator_token = parser_previous_token(parser);

  UnaryOpType operator_type = 0xdeadbeef;
  switch (operator_token.type) {
  case TOKEN_MINUS: operator_type = UNARY_OP_TYPE_MINUS; break;
  case TOKEN_TILDE: operator_type = UNARY_OP_BITWISE_TYPE_COMPLEMENT; break;
  default: MCC_ASSERT_MSG(false, "Unexpected operator");
  }

  // Inner expression
  Expr* expr = parse_precedence(parser, PREC_UNARY);

  // build result
  // TODO: better way to handle the case where expr == NULL
  SourceRange result_source_range =
      expr == NULL ? token_source_range(operator_token)
                   : source_range_union(token_source_range(operator_token),
                                        expr->source_range);

  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *result = (Expr){.type = EXPR_TYPE_UNARY,
                   .source_range = result_source_range,
                   .unary_op = (struct UnaryOpExpr){
                       .unary_op_type = operator_type, .inner_expr = expr}};

  return result;
}

static Expr* parse_binary_op_left_associative(Parser* parser, Expr* lhs_expr)
{
  Token operator_token = parser_previous_token(parser);

  const TokenType operator_type = operator_token.type;
  const ParseRule* rule = get_rule(operator_type);
  Expr* rhs_expr = parse_precedence(parser, (Precedence)(rule->precedence + 1));

  BinaryOpType binary_op_type;
  switch (operator_type) {
  case TOKEN_PLUS: binary_op_type = BINARY_OP_TYPE_PLUS; break;
  case TOKEN_MINUS: binary_op_type = BINARY_OP_TYPE_MINUS; break;
  case TOKEN_STAR: binary_op_type = BINARY_OP_TYPE_MULT; break;
  case TOKEN_SLASH: binary_op_type = BINARY_OP_TYPE_DIVIDE; break;
  case TOKEN_PERCENT: binary_op_type = BINARY_OP_TYPE_MOD; break;
  default: return NULL; // TODO: better error reporting for unreachable
  }

  // build result
  // TODO: Fix this
  SourceRange result_source_range = token_source_range(operator_token);

  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *result = (Expr){.type = EXPR_TYPE_BINARY,
                   .source_range = result_source_range,
                   .binary_op = (struct BinaryOpExpr){
                       .binary_op_type = binary_op_type,
                       .lhs = lhs_expr,
                       .rhs = rhs_expr,
                   }};
  return result;
}

static Expr* parse_expr(Parser* parser)
{
  return parse_precedence(parser, PREC_ASSIGNMENT);
}

static void parse_return_stmt(Parser* parser, ReturnStmt* out_ret_stmt)
{
  Expr* expr = parse_expr(parser);
  parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  *out_ret_stmt = (ReturnStmt){.expr = expr};
}

static void parse_stmt(Parser* parser, Stmt* out_stmt);

// TODO: Implement proper vector and use it for compound statement
#define MAX_STMT_COUNT_IN_COMPOUND_STMT 16

static void parse_compound_stmt(Parser* parser, CompoundStmt* out_compount_stmt)
{
  Stmt* statements = ARENA_ALLOC_ARRAY(parser->permanent_arena, Stmt,
                                       MAX_STMT_COUNT_IN_COMPOUND_STMT);
  size_t statement_count = 0;

  while (parser_current_token(parser).type != TOKEN_RIGHT_BRACE) {
    // TODO: proper handle the case with more compound statements
    if (statement_count == MAX_STMT_COUNT_IN_COMPOUND_STMT) {
      parse_error_at(parser,
                     string_view_from_c_str(
                         "Too many statement in compound statement to handle"),
                     parser_current_token(parser));
      return;
    }

    parse_stmt(parser, &statements[statement_count]);
    ++statement_count;
  }

  parse_consume(parser, TOKEN_RIGHT_BRACE, "Expect }");

  *out_compount_stmt = (CompoundStmt){.statements = statements,
                                      .statement_count = statement_count};
}

static void parse_stmt(Parser* parser, Stmt* out_stmt)
{
  const Token current_token = parser_current_token(parser);
  const SourceLocation first_loc = current_token.location;

  switch (current_token.type) {
  case TOKEN_KEYWORD_RETURN: {
    parse_advance(parser);

    ReturnStmt return_stmt;
    parse_return_stmt(parser, &return_stmt);

    *out_stmt =
        (Stmt){.type = STMT_TYPE_RETURN,
               .source_range = {.begin = first_loc,
                                .end = parser_previous_token(parser).location},
               .ret = return_stmt};

    return;
  }
  case TOKEN_LEFT_BRACE: {
    parse_advance(parser);

    CompoundStmt compound;
    parse_compound_stmt(parser, &compound);

    *out_stmt =
        (Stmt){.type = STMT_TYPE_COMPOUND,
               .source_range = {.begin = first_loc,
                                .end = parser_previous_token(parser).location},
               .compound = compound};
    return;
  }
  default: {
    parse_error_at(parser, string_view_from_c_str("Expect statement"),
                   current_token);
    break;
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
  const Token current_token = parser_current_token(parser);

  if (current_token.type != TOKEN_IDENTIFIER) {
    parse_error_at(parser, string_view_from_c_str("Expect Identifier"),
                   current_token);
  }
  parse_advance(parser);
  return current_token.src;
}

static FunctionDecl* parse_function_decl(Parser* parser)
{
  const SourceLocation first_location = parser_current_token(parser).location;

  parse_consume(parser, TOKEN_KEYWORD_INT, "Expect keyword int");
  StringView function_name = parse_identifier(parser);

  parse_parameter_list(parser);

  CompoundStmt* body = NULL;

  if (parser_current_token(parser).type == TOKEN_LEFT_BRACE) { // is definition
    parse_advance(parser);
    body = ARENA_ALLOC_OBJECT(parser->permanent_arena, CompoundStmt);
    parse_compound_stmt(parser, body);
  } else {
    parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  }

  const SourceLocation last_location = parser_current_token(parser).location;

  FunctionDecl* decl =
      ARENA_ALLOC_OBJECT(parser->permanent_arena, FunctionDecl);
  *decl = (FunctionDecl){
      .source_range = {.begin = first_location, .end = last_location},
      .name = function_name,
      .body = body,
  };
  return decl;
}

static TranslationUnit* parse_translation_unit(Parser* parser)
{
  FunctionDecl* decl = parse_function_decl(parser);

  TranslationUnit* tu =
      ARENA_ALLOC_OBJECT(parser->permanent_arena, TranslationUnit);
  *tu = (TranslationUnit){
      .decl_count = 1,
      .decls = decl,
  };
  parse_consume(parser, TOKEN_EOF, "Expect end of the file");

  return tu;
}

ParseResult parse(Tokens tokens, Arena* permanent_arena, Arena scratch_arena)
{
  Parser parser = {.permanent_arena = permanent_arena,
                   .scratch_arena = scratch_arena,
                   .tokens = tokens,
                   .current_token_index = 0,
                   .has_error = false,
                   .in_panic_mode = false,
                   .errors = {
                       .size = 0,
                       .data = NULL,
                   }};

  TranslationUnit* tu = parse_translation_unit(&parser);

  return (ParseResult){.ast = parser.has_error ? NULL : tu,
                       .errors = parser.errors};
}
