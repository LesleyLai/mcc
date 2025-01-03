#include <mcc/parser.h>

#include <mcc/ast.h>
#include <mcc/dynarray.h>
#include <mcc/format.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct ParseErrorVec {
  size_t length;
  size_t capacity;
  ParseError* data;
};

typedef struct Parser {
  Arena* permanent_arena;
  Arena scratch_arena;

  Tokens tokens;
  int current_token_index;

  bool has_error;
  bool in_panic_mode;

  struct ParseErrorVec errors;
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

  ParseError error =
      (ParseError){.msg = error_msg, .range = token_source_range(token)};
  DYNARRAY_PUSH_BACK(&parser->errors, ParseError, parser->permanent_arena,
                     error);

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
  *result = (Expr){.type = EXPR_CONST,
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
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // ||
  PREC_AND,         // &&
  PREC_BITWISE_OR,  // |
  PREC_BITWISE_XOR, // ^
  PREC_BITWISE_AND, // &
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_SHIFT,       // << >>
  PREC_TERM,        // + -
  PREC_FACTOR,      // * / %
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
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

static ParseRule rules[TOKEN_TYPES_COUNT] = {
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
    [TOKEN_AMPERSAND] = {NULL, parse_binary_op_left_associative,
                         PREC_BITWISE_AND},
    [TOKEN_CARET] = {NULL, parse_binary_op_left_associative, PREC_BITWISE_XOR},
    [TOKEN_BAR] = {NULL, parse_binary_op_left_associative, PREC_BITWISE_OR},

    [TOKEN_AMPERSAND_AMPERSAND] = {NULL, parse_binary_op_left_associative,
                                   PREC_AND},
    [TOKEN_BAR_BAR] = {NULL, parse_binary_op_left_associative, PREC_OR},
    [TOKEN_EQUAL_EQUAL] = {NULL, parse_binary_op_left_associative,
                           PREC_EQUALITY},
    [TOKEN_NOT] = {parse_unary_op, NULL, PREC_UNARY},
    [TOKEN_NOT_EQUAL] = {NULL, parse_binary_op_left_associative, PREC_EQUALITY},

    [TOKEN_LESS] = {NULL, parse_binary_op_left_associative, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, parse_binary_op_left_associative,
                          PREC_COMPARISON},
    [TOKEN_LESS_LESS] = {NULL, parse_binary_op_left_associative, PREC_SHIFT},
    [TOKEN_GREATER] = {NULL, parse_binary_op_left_associative, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, parse_binary_op_left_associative,
                             PREC_COMPARISON},
    [TOKEN_GREATER_GREATER] = {NULL, parse_binary_op_left_associative,
                               PREC_SHIFT},
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

  UnaryOpType operator_type;
  switch (operator_token.type) {
  case TOKEN_MINUS: operator_type = UNARY_OP_NEGATION; break;
  case TOKEN_TILDE: operator_type = UNARY_OP_BITWISE_TYPE_COMPLEMENT; break;
  case TOKEN_NOT: operator_type = UNARY_OP_NOT; break;
  default: MCC_UNREACHABLE();
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
  *result = (Expr){.type = EXPR_UNARY,
                   .source_range = result_source_range,
                   .unary_op = (struct UnaryOpExpr){
                       .unary_op_type = operator_type, .inner_expr = expr}};

  return result;
}

static BinaryOpType binop_type_from_token_type(TokenType token_type)
{
  switch (token_type) {
  case TOKEN_PLUS: return BINARY_OP_PLUS;
  case TOKEN_MINUS: return BINARY_OP_MINUS;
  case TOKEN_STAR: return BINARY_OP_MULT;
  case TOKEN_SLASH: return BINARY_OP_DIVIDE;
  case TOKEN_PERCENT: return BINARY_OP_MOD;
  case TOKEN_LESS_LESS: return BINARY_OP_SHIFT_LEFT;
  case TOKEN_GREATER_GREATER: return BINARY_OP_SHIFT_RIGHT;
  case TOKEN_AMPERSAND: return BINARY_OP_BITWISE_AND;
  case TOKEN_CARET: return BINARY_OP_BITWISE_XOR;
  case TOKEN_BAR: return BINARY_OP_BITWISE_OR;
  case TOKEN_AMPERSAND_AMPERSAND: return BINARY_OP_AND;
  case TOKEN_BAR_BAR: return BINARY_OP_OR;
  case TOKEN_EQUAL_EQUAL: return BINARY_OP_EQUAL;
  case TOKEN_NOT_EQUAL: return BINARY_OP_NOT_EQUAL;
  case TOKEN_LESS: return BINARY_OP_LESS;
  case TOKEN_LESS_EQUAL: return BINARY_OP_LESS_EQUAL;
  case TOKEN_GREATER: return BINARY_OP_GREATER;
  case TOKEN_GREATER_EQUAL: return BINARY_OP_GREATER_EQUAL;
  default: MCC_UNREACHABLE();
  }
}

static Expr* parse_binary_op_left_associative(Parser* parser, Expr* lhs_expr)
{
  Token operator_token = parser_previous_token(parser);

  const TokenType operator_type = operator_token.type;
  const ParseRule* rule = get_rule(operator_type);
  Expr* rhs_expr = parse_precedence(parser, (Precedence)(rule->precedence + 1));

  BinaryOpType binary_op_type = binop_type_from_token_type(operator_type);

  // build result
  // TODO: Fix this
  SourceRange result_source_range = token_source_range(operator_token);

  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *result = (Expr){.type = EXPR_BINARY,
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

static ReturnStmt parse_return_stmt(Parser* parser)
{
  Expr* expr = parse_expr(parser);
  parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  return (ReturnStmt){.expr = expr};
}

static void parse_stmt(Parser* parser, Stmt* out_stmt);

struct StmtVector {
  size_t capacity;
  size_t length;
  Stmt* data;
};

static CompoundStmt parse_compound_stmt(Parser* parser)
{
  struct StmtVector stmt_vector = {};

  Arena scratch_arena = parser->scratch_arena;

  bool has_error = false;

  while (parser_current_token(parser).type != TOKEN_RIGHT_BRACE) {
    Stmt stmt;
    parse_stmt(parser, &stmt);

    if (parser->in_panic_mode) {
      has_error = true;
      break;
    } else {
      DYNARRAY_PUSH_BACK(&stmt_vector, Stmt, &scratch_arena, stmt);
    }
  }

  if (has_error) {
    parser->in_panic_mode = false;
    while (parser_current_token(parser).type != TOKEN_EOF) {
      const TokenType previous_type = parser_previous_token(parser).type;
      if (previous_type != TOKEN_RIGHT_BRACE &&
          previous_type != TOKEN_SEMICOLON) {
        break;
      }
      parse_advance(parser);
    }
  } else {
    parse_consume(parser, TOKEN_RIGHT_BRACE, "Expect }");
  }

  Stmt* stmts =
      ARENA_ALLOC_ARRAY(parser->permanent_arena, Stmt, stmt_vector.length);
  if (stmt_vector.length != 0) {
    memcpy(stmts, stmt_vector.data, stmt_vector.length * sizeof(Stmt));
  }

  return (CompoundStmt){.statements = stmts,
                        .statement_count = stmt_vector.length};
}

static void parse_stmt(Parser* parser, Stmt* out_stmt)
{
  const Token current_token = parser_current_token(parser);
  const SourceLocation first_loc = current_token.location;

  switch (current_token.type) {
  case TOKEN_KEYWORD_RETURN: {
    parse_advance(parser);

    ReturnStmt return_stmt = parse_return_stmt(parser);

    *out_stmt =
        (Stmt){.type = STMT_RETURN,
               .source_range = {.begin = first_loc,
                                .end = parser_previous_token(parser).location},
               .ret = return_stmt};

    return;
  }
  case TOKEN_LEFT_BRACE: {
    parse_advance(parser);

    CompoundStmt compound = parse_compound_stmt(parser);

    *out_stmt =
        (Stmt){.type = STMT_COMPOUND,
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
    *body = parse_compound_stmt(parser);
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
  Parser parser = {
      .permanent_arena = permanent_arena,
      .scratch_arena = scratch_arena,
      .tokens = tokens,
  };

  TranslationUnit* tu = parse_translation_unit(&parser);

  const bool has_error = parser.errors.data != NULL;
  const ParseErrorsView errors = (ParseErrorsView){
      .length = parser.errors.length,
      .data = parser.errors.data,
  };
  return (ParseResult){.ast = has_error ? NULL : tu, .errors = errors};
}
