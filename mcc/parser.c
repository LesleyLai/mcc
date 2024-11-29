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

// static SourceRange source_range_union(SourceRange lhs, SourceRange rhs)
//{
//   return (SourceRange){
//       .begin = (lhs.begin.offset < rhs.begin.offset) ? lhs.begin : rhs.begin,
//       .end = (lhs.end.offset > rhs.end.offset) ? lhs.end : rhs.end};
// }

static void parse_error_at(Parser* parser, StringView error_msg, Token token)
{
  if (parser->in_panic_mode) return;

  // TODO: proper error handling
  if (parser->errors.size >= MAX_ERROR_COUNT) {
    fprintf(stderr, "Too many data for mcc to handle");
    fflush(stderr);
    exit(1);
  }

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
    fprintf(stderr, "Negative token index %i!\n", token_index);
    exit(1);
  }

  if (parser->tokens.begin + token_index >= parser->tokens.end) {
    fprintf(stderr, "Token index out of bound: %i (total %li)!\n", token_index,
            parser->tokens.end - parser->tokens.begin);
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
  *result = (Expr){.type = CONST_EXPR,
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
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

static Expr* parse_expr(Parser* parser);
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

  const ParseFn prefix_rule =
      get_rule(parser_previous_token(parser).type)->prefix;
  if (prefix_rule == NULL) {
    parse_error_at(parser, string_view_from_c_str("Expect valid expression"),
                   parser_current_token(parser));
    return NULL;
  }

  Expr* expr = prefix_rule(parser);

  while (precedence <=
         get_rule(parser_current_token(parser).type)->precedence) {
    parse_advance(parser);
    // ParseFn infix_rule = get_rule(parser->previous.type)->infix;
    //  Expr* rhs = infix_rule(parser);
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
  TokenType operator_type = parser_previous_token(parser).type;

  Expr* expr = parse_precedence(parser, PREC_UNARY);

  switch (operator_type) {
  case TOKEN_MINUS: return expr; // TODO: should actually wrap it
  default: return NULL;          // TODO: better error reporting for unreachable
  }
}

static Expr* parse_binary_op(Parser* parser)
{
  const TokenType operator_type = parser_previous_token(parser).type;
  const ParseRule* rule = get_rule(operator_type);
  parse_precedence(parser, (Precedence)(rule->precedence + 1));

  //  BinaryOpType binary_op_type;
  //  switch (operator_type) {
  //  case TOKEN_PLUS: binary_op_type = BINARY_OP_PLUS; break;
  //  case TOKEN_MINUS: binary_op_type = BINARY_OP_MINUS; break;
  //  case TOKEN_STAR: binary_op_type = BINARY_OP_MULT; break;
  //  case TOKEN_SLASH: binary_op_type = BINARY_OP_DIVIDE; break;
  //  default: return NULL; // TODO: better error reporting for unreachable
  //  }

  return NULL;
}

static Expr* parse_expr(Parser* parser)
{
  return parse_precedence(parser, PREC_ASSIGNMENT);
}

static void parse_return_stmt(Parser* parser, ReturnStmt* out_ret_stmt)
{
  parse_advance(parser);
  Expr* expr = parse_expr(parser);
  if (parser->in_panic_mode) return;

  assert(expr != NULL);

  parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  *out_ret_stmt = (ReturnStmt){.expr = expr};
}

static void parse_stmt(Parser* parser, Stmt* out_stmt);

// TODO: Implement proper vector and use it for compound statement
#define MAX_STMT_COUNT_IN_COMPOUND_STMT 16

static void parse_compound_stmt(Parser* parser, CompoundStmt* out_compount_stmt
                                /*SourceLocation first_loc*/)
{
  Stmt* statements = ARENA_ALLOC_ARRAY(parser->permanent_arena, Stmt,
                                       MAX_STMT_COUNT_IN_COMPOUND_STMT);
  size_t statement_count = 0;

  bool encounter_error = false;
  while (parser_current_token(parser).type != TOKEN_RIGHT_BRACE) {
    if (statement_count == MAX_STMT_COUNT_IN_COMPOUND_STMT) {
      parse_error_at(parser,
                     string_view_from_c_str(
                         "Too many statement in compound statement to handle"),
                     parser_current_token(parser));
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
    while (parser_current_token(parser).type != TOKEN_RIGHT_BRACE) {
      parse_advance(parser);
    }
  }

  parse_consume(parser, TOKEN_RIGHT_BRACE, "Expect }");

  *out_compount_stmt = (CompoundStmt){.statements = statements,
                                      .statement_count = statement_count};

  // const Token previous_token = parser_previous_token(parser);
  // const SourceLocation last_loc = {
  //     .line = previous_token.location.line,
  //     .column = (uint32_t)(previous_token.location.column +
  //                          previous_token.src.size - 1),
  // };

  // Stmt* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Stmt);
  // *result = (Stmt){.type = COMPOUND_STMT,
  //                  .source_range = {.first = first_loc, .last = last_loc},
  //                  .compound = {.statements = statements,
  //                               .statement_count = statement_count}};
  // return result;
}

static void parse_stmt(Parser* parser, Stmt* out_stmt)
{
  const Token current_token = parser_current_token(parser);
  const SourceLocation first_loc = current_token.location;

  switch (current_token.type) {
  case TOKEN_KEYWORD_RETURN: {
    parse_advance(parser);

    // Stmt* stmt = ARENA_ALLOC_OBJECT(parser->ast_arena, Stmt);
    ReturnStmt return_stmt;
    parse_return_stmt(parser, &return_stmt);
    *out_stmt =
        (Stmt){.type = RETURN_STMT,
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
        (Stmt){.type = COMPOUND_STMT,
               .source_range = {.begin = first_loc,
                                .end = parser_previous_token(parser).location},
               .compound = compound};
    return;
  }
  default: {
    parse_error_at(parser, string_view_from_c_str("Expect statement"),
                   current_token);
    return;
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
  if (parser->in_panic_mode) { return NULL; }

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
  if (parser->in_panic_mode) { return NULL; }

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
