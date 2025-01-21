#include <mcc/frontend.h>

#include <mcc/ast.h>
#include <mcc/dynarray.h>
#include <mcc/format.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct ParseErrorVec {
  size_t length;
  size_t capacity;
  Error* data;
};

typedef struct Parser {
  const char* src;

  Arena* permanent_arena;
  Arena scratch_arena;

  Tokens tokens;
  uint32_t current_token_index;

  bool has_error;
  bool in_panic_mode;

  struct ParseErrorVec errors;
} Parser;

static SourceRange token_source_range(Token token)
{
  const uint32_t begin =
      (token.type == TOKEN_EOF) ? token.start - 1 : token.start;

  return (SourceRange){.begin = begin, .end = token.start + token.size};
}

static SourceRange source_range_union(SourceRange lhs, SourceRange rhs)
{
  return (SourceRange){.begin = (lhs.begin < rhs.begin) ? lhs.begin : rhs.begin,
                       .end = (lhs.end > rhs.end) ? lhs.end : rhs.end};
}

static void parse_error_at(Parser* parser, StringView error_msg,
                           SourceRange range)
{
  if (parser->in_panic_mode) return;

  Error error = (Error){.msg = error_msg, .range = range};
  DYNARRAY_PUSH_BACK(&parser->errors, Error, parser->permanent_arena, error);

  parser->in_panic_mode = true;
  parser->has_error = true;
}

static void parse_error_at_token(Parser* parser, StringView error_msg,
                                 Token token)
{
  parse_error_at(parser, error_msg, token_source_range(token));
}

// gets the current token
static Token parser_current_token(Parser* parser)
{
  return get_token(&parser->tokens, parser->current_token_index);
}

// gets the current token
static Token parser_previous_token(Parser* parser)
{
  MCC_ASSERT(parser->current_token_index > 0);
  const uint32_t previous_token_index = parser->current_token_index - 1;
  return get_token(&parser->tokens, previous_token_index);
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
    parse_error_at_token(parser, str("unexpected character"), current);
  }
}

// Consume the current token. If the token doesn't have specified type, generate
// an error.
static void parse_consume(Parser* parser, TokenType type, const char* error_msg)
{
  const Token current = parser_current_token(parser);

  if (current.type != type) {
    parse_error_at_token(parser, str(error_msg), current);
  }

  parse_advance(parser);
}

/* =============================================================================
 * Local Variables are accumulated here
 * =============================================================================
 */

struct Variables {
  uint32_t length;
  uint32_t capacity;
  StringView* data;
};

// TODO: use hash table
typedef struct VariableMap {
  uint32_t length;
  uint32_t capacity;
  StringView* data;
  struct VariableMap* parent;
} VariableMap;

static struct VariableMap* new_variable_map(VariableMap* parent, Arena* arena)
{
  struct VariableMap* map = ARENA_ALLOC_OBJECT(arena, struct VariableMap);
  *map = (struct VariableMap){
      .parent = parent,
  };
  return map;
}

static bool lookup_variable(const VariableMap* map, StringView var)
{
  for (uint32_t i = 0; i < map->length; ++i) {
    if (str_eq(var, map->data[i])) { return true; }
  }
  return map->parent != nullptr && lookup_variable(map->parent, var);
}

// Return false if a local variable already exist
static bool add_variable(StringView var, VariableMap* map, Arena* arena)
{
  if (lookup_variable(map, var)) { return false; }
  DYNARRAY_PUSH_BACK(map, StringView, arena, var);
  return true;
}

// =============================================================================
// Expression parsing
// =============================================================================
static Expr* parse_number_literal(Parser* parser, VariableMap* scope)
{
  (void)scope;

  const Token token = parser_previous_token(parser);

  char* end_ptr;
  const int val = (int)strtol(parser->src + token.start, &end_ptr,
                              10); // TODO(llai): replace strtol

  MCC_ASSERT_MSG(end_ptr == parser->src + token.start + token.size,
                 "Not used all characters for numbers");

  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *result = (Expr){.tag = EXPR_CONST,
                   .source_range = token_source_range(token),
                   .const_expr = (struct ConstExpr){.val = val}};
  return result;
}

static Expr* parse_identifier_expr(Parser* parser, VariableMap* scope)
{
  const Token token = parser_previous_token(parser);

  MCC_ASSERT(token.type == TOKEN_IDENTIFIER);

  const StringView identifier = (struct StringView){
      .start = parser->src + token.start,
      .size = token.size,
  };

  // TODO: handle typedef
  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);

  // If local variable does not exist
  if (!lookup_variable(scope, identifier)) {
    const StringView error_msg = allocate_printf(
        parser->permanent_arena, "use of undeclared identifier '%.*s'",
        (int)identifier.size, identifier.start);
    parse_error_at_token(parser, error_msg, token);
    *result =
        (Expr){.tag = EXPR_INVALID, .source_range = token_source_range(token)};
  } else {
    *result = (Expr){.tag = EXPR_VARIABLE,
                     .source_range = token_source_range(token),
                     .variable = identifier};
  }
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

static Expr* parse_expr(Parser* parser, struct VariableMap* scope);
static Expr* parse_group(Parser* parser, struct VariableMap* scope);
static Expr* parse_unary_op(Parser* parser, struct VariableMap* scope);
static Expr* parse_binop_left(Parser* parser, Expr* lhs_expr,
                              struct VariableMap* scope); // left associative
static Expr* parse_assignment(Parser* parser, Expr* lhs_expr,
                              struct VariableMap* scope); // right associative

typedef Expr* (*PrefixParseFn)(Parser*, struct VariableMap* scope);
typedef Expr* (*InfixParseFn)(Parser*, Expr*, struct VariableMap* scope);

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
    [TOKEN_PLUS] = {NULL, parse_binop_left, PREC_TERM},
    [TOKEN_PLUS_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_MINUS] = {parse_unary_op, parse_binop_left, PREC_TERM},
    [TOKEN_MINUS_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_STAR] = {NULL, parse_binop_left, PREC_FACTOR},
    [TOKEN_STAR_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_SLASH] = {NULL, parse_binop_left, PREC_FACTOR},
    [TOKEN_SLASH_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_PERCENT] = {NULL, parse_binop_left, PREC_FACTOR},
    [TOKEN_PERCENT_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_AMPERSAND] = {NULL, parse_binop_left, PREC_BITWISE_AND},
    [TOKEN_AMPERSAND_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_AMPERSAND_AMPERSAND] = {NULL, parse_binop_left, PREC_AND},
    [TOKEN_BAR] = {NULL, parse_binop_left, PREC_BITWISE_OR},
    [TOKEN_BAR_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_BAR_BAR] = {NULL, parse_binop_left, PREC_OR},
    [TOKEN_CARET] = {NULL, parse_binop_left, PREC_BITWISE_XOR},
    [TOKEN_CARET_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_EQUAL_EQUAL] = {NULL, parse_binop_left, PREC_EQUALITY},
    [TOKEN_NOT] = {parse_unary_op, NULL, PREC_UNARY},
    [TOKEN_NOT_EQUAL] = {NULL, parse_binop_left, PREC_EQUALITY},
    [TOKEN_LESS] = {NULL, parse_binop_left, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, parse_binop_left, PREC_COMPARISON},
    [TOKEN_LESS_LESS] = {NULL, parse_binop_left, PREC_SHIFT},
    [TOKEN_LESS_LESS_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_GREATER] = {NULL, parse_binop_left, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, parse_binop_left, PREC_COMPARISON},
    [TOKEN_GREATER_GREATER] = {NULL, parse_binop_left, PREC_SHIFT},
    [TOKEN_GREATER_GREATER_EQUAL] = {NULL, parse_assignment, PREC_ASSIGNMENT},
    [TOKEN_TILDE] = {parse_unary_op, NULL, PREC_TERM},
    [TOKEN_KEYWORD_VOID] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_INT] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {parse_identifier_expr, NULL, PREC_NONE},
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

static Expr* parse_precedence(Parser* parser, Precedence precedence,
                              struct VariableMap* scope)
{
  parse_advance(parser);

  const Token previous_token = parser_previous_token(parser);

  const PrefixParseFn prefix_rule = get_rule(previous_token.type)->prefix;
  if (prefix_rule == NULL) {
    parse_error_at_token(parser, str("Expect valid expression"),
                         previous_token);
    Expr* error_expr = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
    *error_expr = (Expr){
        .tag = EXPR_INVALID,
        .source_range = token_source_range(previous_token),
    };
    return error_expr;
  }

  Expr* expr = prefix_rule(parser, scope);

  while (precedence <=
         get_rule(parser_current_token(parser).type)->precedence) {
    parse_advance(parser);
    InfixParseFn infix_rule =
        get_rule(parser_previous_token(parser).type)->infix;
    expr = infix_rule(parser, expr, scope);
  }

  return expr;
}

static Expr* parse_group(Parser* parser, struct VariableMap* scope)
{
  Expr* result = parse_expr(parser, scope);
  parse_consume(parser, TOKEN_RIGHT_PAREN, "Expect )");
  return result;
}

static Expr* parse_unary_op(Parser* parser, VariableMap* scope)
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
  Expr* expr = parse_precedence(parser, PREC_UNARY, scope);

  // build result
  // TODO: better way to handle the case where expr == NULL
  SourceRange result_source_range =
      expr == NULL ? token_source_range(operator_token)
                   : source_range_union(token_source_range(operator_token),
                                        expr->source_range);

  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *result = (Expr){.tag = EXPR_UNARY,
                   .source_range = result_source_range,
                   .unary_op = (struct UnaryOpExpr){
                       .unary_op_type = operator_type, .inner_expr = expr}};

  return result;
}

static BinaryOpType binop_type_from_token_type(TokenType token_type)
{
  switch (token_type) {
  case TOKEN_PLUS: return BINARY_OP_PLUS;
  case TOKEN_PLUS_EQUAL: return BINARY_OP_PLUS_EQUAL;
  case TOKEN_MINUS: return BINARY_OP_MINUS;
  case TOKEN_MINUS_EQUAL: return BINARY_OP_MINUS_EQUAL;
  case TOKEN_STAR: return BINARY_OP_MULT;
  case TOKEN_STAR_EQUAL: return BINARY_OP_MULT_EQUAL;
  case TOKEN_SLASH: return BINARY_OP_DIVIDE;
  case TOKEN_SLASH_EQUAL: return BINARY_OP_DIVIDE_EQUAL;
  case TOKEN_PERCENT: return BINARY_OP_MOD;
  case TOKEN_PERCENT_EQUAL: return BINARY_OP_MOD_EQUAL;
  case TOKEN_LESS_LESS: return BINARY_OP_SHIFT_LEFT;
  case TOKEN_LESS_LESS_EQUAL: return BINARY_OP_SHIFT_LEFT_EQUAL;
  case TOKEN_GREATER_GREATER: return BINARY_OP_SHIFT_RIGHT;
  case TOKEN_GREATER_GREATER_EQUAL: return BINARY_OP_SHIFT_RIGHT_EQUAL;
  case TOKEN_AMPERSAND: return BINARY_OP_BITWISE_AND;
  case TOKEN_AMPERSAND_EQUAL: return BINARY_OP_BITWISE_AND_EQUAL;
  case TOKEN_CARET: return BINARY_OP_BITWISE_XOR;
  case TOKEN_CARET_EQUAL: return BINARY_OP_BITWISE_XOR_EQUAL;
  case TOKEN_BAR: return BINARY_OP_BITWISE_OR;
  case TOKEN_BAR_EQUAL: return BINARY_OP_BITWISE_OR_EQUAL;
  case TOKEN_AMPERSAND_AMPERSAND: return BINARY_OP_AND;
  case TOKEN_BAR_BAR: return BINARY_OP_OR;
  case TOKEN_EQUAL: return BINARY_OP_ASSIGNMENT;
  case TOKEN_EQUAL_EQUAL: return BINARY_OP_EQUAL;
  case TOKEN_NOT_EQUAL: return BINARY_OP_NOT_EQUAL;
  case TOKEN_LESS: return BINARY_OP_LESS;
  case TOKEN_LESS_EQUAL: return BINARY_OP_LESS_EQUAL;
  case TOKEN_GREATER: return BINARY_OP_GREATER;
  case TOKEN_GREATER_EQUAL: return BINARY_OP_GREATER_EQUAL;
  default: MCC_UNREACHABLE();
  }
}

enum Associativity { ASSOCIATIVITY_LEFT, ASSOCIATIVITY_RIGHT };

static Expr* parse_binary_op(Parser* parser, Expr* lhs_expr,
                             enum Associativity associativity,
                             VariableMap* scope)
{
  Token operator_token = parser_previous_token(parser);

  const TokenType operator_type = operator_token.type;
  const ParseRule* rule = get_rule(operator_type);
  Expr* rhs_expr = parse_precedence(
      parser,
      (Precedence)(rule->precedence +
                   ((associativity == ASSOCIATIVITY_LEFT) ? 1 : 0)),
      scope);

  BinaryOpType binary_op_type = binop_type_from_token_type(operator_type);

  // build result
  const SourceRange result_source_range =
      source_range_union(source_range_union(token_source_range(operator_token),
                                            lhs_expr->source_range),
                         rhs_expr->source_range);

  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *result = (Expr){.tag = EXPR_BINARY,
                   .source_range = result_source_range,
                   .binary_op = (struct BinaryOpExpr){
                       .binary_op_type = binary_op_type,
                       .lhs = lhs_expr,
                       .rhs = rhs_expr,
                   }};
  return result;
}

static Expr* parse_binop_left(Parser* parser, Expr* lhs_expr,
                              VariableMap* scope)
{
  return parse_binary_op(parser, lhs_expr, ASSOCIATIVITY_LEFT, scope);
}

static Expr* parse_assignment(Parser* parser, Expr* lhs_expr,
                              VariableMap* scope)
{
  // Check whether the left hand side is lvalue
  if (lhs_expr->tag != EXPR_VARIABLE) {
    parse_error_at(parser, str("expression is not assignable"),
                   lhs_expr->source_range);
  }
  return parse_binary_op(parser, lhs_expr, ASSOCIATIVITY_RIGHT, scope);
}

static Expr* parse_expr(Parser* parser, VariableMap* scope)
{
  return parse_precedence(parser, PREC_ASSIGNMENT, scope);
}

static ReturnStmt parse_return_stmt(Parser* parser, VariableMap* scope)
{
  Expr* expr = parse_expr(parser, scope);
  parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  return (ReturnStmt){.expr = expr};
}

static Stmt parse_stmt(Parser* parser, struct VariableMap* scope);

struct BlockItemVec {
  size_t capacity;
  size_t length;
  BlockItem* data;
};

static StringView str_from_token(const char* src, Token token)
{
  return (StringView){.start = src + token.start, .size = token.size};
}

static VariableDecl parse_decl(Parser* parser, struct VariableMap* scope)
{
  const Token name = parser_current_token(parser);
  // TODO: proper error handling
  MCC_ASSERT(name.type == TOKEN_IDENTIFIER);

  const StringView identifier = str_from_token(parser->src, name);
  if (!add_variable(identifier, scope, parser->permanent_arena)) {
    const StringView error_msg =
        allocate_printf(parser->permanent_arena, "redefinition of '%.*s'",
                        (int)identifier.size, identifier.start);
    parse_error_at_token(parser, error_msg, name);
  }

  parse_advance(parser);

  const Expr* initializer = nullptr;
  if (parser_current_token(parser).type == TOKEN_EQUAL) {
    parse_advance(parser);
    initializer = parse_expr(parser, scope);
  }
  parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");

  return (VariableDecl){.name = identifier, .initializer = initializer};
}

static BlockItem parse_block_item(Parser* parser, struct VariableMap* scope)
{
  const Token current_token = parser_current_token(parser);
  if (current_token.type == TOKEN_KEYWORD_INT) {
    parse_advance(parser);
    return (BlockItem){.tag = BLOCK_ITEM_DECL,
                       .decl = parse_decl(parser, scope)};
  } else {
    return (BlockItem){.tag = BLOCK_ITEM_STMT,
                       .stmt = parse_stmt(parser, scope)};
  }
}

static Block parse_block(Parser* parser, struct VariableMap* parent_scope)
{
  struct VariableMap* scope =
      new_variable_map(parent_scope, parser->permanent_arena);

  struct BlockItemVec items_vec = {};

  bool has_error = false;

  while (parser_current_token(parser).type != TOKEN_RIGHT_BRACE &&
         parser_current_token(parser).type != TOKEN_EOF) {
    BlockItem item = parse_block_item(parser, scope);
    DYNARRAY_PUSH_BACK(&items_vec, BlockItem, &parser->scratch_arena, item);

    if (parser->in_panic_mode) {
      has_error = true;
      break;
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
    parse_consume(parser, TOKEN_RIGHT_BRACE, "Expect `}`");
  }

  BlockItem* block_items =
      ARENA_ALLOC_ARRAY(parser->permanent_arena, BlockItem, items_vec.length);
  if (items_vec.length != 0) {
    memcpy(block_items, items_vec.data, items_vec.length * sizeof(BlockItem));
  }

  return (Block){.children = block_items, .child_count = items_vec.length};
}

static Stmt parse_stmt(Parser* parser, struct VariableMap* scope)
{
  const Token start_token = parser_current_token(parser);

  switch (start_token.type) {
  case TOKEN_SEMICOLON: {
    parse_advance(parser);

    return (Stmt){.tag = STMT_EMPTY,
                  .source_range = token_source_range(start_token)};
  }

  case TOKEN_KEYWORD_RETURN: {
    parse_advance(parser);

    ReturnStmt return_stmt = parse_return_stmt(parser, scope);

    return (Stmt){.tag = STMT_RETURN,
                  .source_range = {.begin = start_token.start,
                                   .end = parser_previous_token(parser).start},
                  .ret = return_stmt};
  }
  case TOKEN_LEFT_BRACE: {
    parse_advance(parser);

    const Block compound = parse_block(parser, scope);

    return (Stmt){.tag = STMT_COMPOUND,
                  .source_range = {.begin = start_token.start,
                                   .end = parser_previous_token(parser).start},
                  .compound = compound};
  }
  default: {
    const Expr* expr = parse_expr(parser, scope);
    parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");

    return (Stmt){.tag = STMT_EXPR,
                  .expr = expr,
                  .source_range = source_range_union(
                      token_source_range(start_token),
                      token_source_range(parser_previous_token(parser)))};
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
    parse_error_at_token(parser, str("Expect Identifier"), current_token);
  }
  parse_advance(parser);
  return (StringView){.start = parser->src + current_token.start,
                      .size = current_token.size};
}

static FunctionDecl* parse_function_decl(Parser* parser)
{
  const uint32_t start_offset = parser_current_token(parser).start;

  parse_consume(parser, TOKEN_KEYWORD_INT, "Expect keyword int");
  StringView function_name = parse_identifier(parser);

  parse_parameter_list(parser);

  Block* body = NULL;

  if (parser_current_token(parser).type == TOKEN_LEFT_BRACE) { // is definition
    parse_advance(parser);
    body = ARENA_ALLOC_OBJECT(parser->permanent_arena, Block);
    *body = parse_block(parser, nullptr);
  } else {
    parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  }

  FunctionDecl* decl =
      ARENA_ALLOC_OBJECT(parser->permanent_arena, FunctionDecl);
  *decl = (FunctionDecl){
      .source_range = {.begin = start_offset,
                       .end = parser_current_token(parser).start},
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

ParseResult parse(const char* src, Tokens tokens, Arena* permanent_arena,
                  Arena scratch_arena)
{
  Parser parser = {.src = src,
                   .tokens = tokens,
                   .permanent_arena = permanent_arena,
                   .scratch_arena = scratch_arena};

  TranslationUnit* tu = parse_translation_unit(&parser);

  const bool has_error = parser.errors.data != NULL;
  const ErrorsView errors = (ErrorsView){
      .length = parser.errors.length,
      .data = parser.errors.data,
  };
  return (ParseResult){.ast = has_error ? NULL : tu, .errors = errors};
}
