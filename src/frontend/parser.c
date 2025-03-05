#include <mcc/frontend.h>

#include <mcc/ast.h>
#include <mcc/dynarray.h>
#include <mcc/format.h>

#include <stdio.h>
#include <string.h>

#include "symbol_table.h"

struct ErrorVec {
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

  struct ErrorVec errors;

  struct Scope* global_scope;
  HashMap functions;
} Parser;

#pragma region source range operations
static SourceRange token_source_range(Token token)
{
  const uint32_t begin =
      (token.tag == TOKEN_EOF) ? token.start - 1 : token.start;

  return (SourceRange){.begin = begin, .end = token.start + token.size};
}

static SourceRange source_range_union(SourceRange lhs, SourceRange rhs)
{
  return (SourceRange){.begin = (lhs.begin < rhs.begin) ? lhs.begin : rhs.begin,
                       .end = (lhs.end > rhs.end) ? lhs.end : rhs.end};
}
#pragma endregion

#pragma region source error handling
// A version of error that does not cause parser to enter panic mode
static void parse_error_at(Parser* parser, StringView error_msg,
                           SourceRange range)
{
  if (parser->in_panic_mode) return;

  Error error = (Error){.msg = error_msg, .range = range};
  DYNARRAY_PUSH_BACK(&parser->errors, Error, parser->permanent_arena, error);

  parser->has_error = true;
}

static void parse_panic_at(Parser* parser, StringView error_msg,
                           SourceRange range)
{
  if (parser->in_panic_mode) return;

  Error error = (Error){.msg = error_msg, .range = range};
  DYNARRAY_PUSH_BACK(&parser->errors, Error, parser->permanent_arena, error);

  parser->in_panic_mode = true;
  parser->has_error = true;
}

static void parse_panic_at_token(Parser* parser, StringView error_msg,
                                 Token token)
{
  parse_panic_at(parser, error_msg, token_source_range(token));
}
#pragma endregion

#pragma region parser helpers

static StringView str_from_token(const char* src, Token token)
{
  return (StringView){.start = src + token.start, .size = token.size};
}

// gets the current token
static Token parser_current_token(const Parser* parser)
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

static bool token_match_or_eof(const Parser* parser, TokenTag typ)
{
  const Token current_token = parser_current_token(parser);
  return current_token.tag == typ || current_token.tag == TOKEN_EOF;
}

// Advance tokens by one
// Also skip any error tokens
static void parse_advance(Parser* parser)
{
  if (parser_current_token(parser).tag == TOKEN_EOF) { return; }

  for (;;) {
    parser->current_token_index++;
    Token current = parser_current_token(parser);
    if (current.tag != TOKEN_ERROR) break;
    parse_panic_at_token(parser, str("unexpected character"), current);
  }
}

// Consume the current token. If the token doesn't have specified type, generate
// an error.
static void parse_consume(Parser* parser, TokenTag type, const char* error_msg)
{
  const Token current = parser_current_token(parser);

  if (current.tag == type) {
    parse_advance(parser);
    return;
  }

  parse_panic_at_token(parser, str(error_msg), current);
}

static Token parse_identifier(Parser* parser)
{
  const Token token = parser_current_token(parser);

  if (token.tag != TOKEN_IDENTIFIER) {
    parse_panic_at_token(parser, str("Expect Identifier"), token);
  }
  parse_advance(parser);
  return token;
}
#pragma endregion

#pragma region Expression parsing
static Expr* parse_number_literal(Parser* parser, Scope* scope)
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

static Expr* parse_identifier_expr(Parser* parser, Scope* scope)
{
  const Token token = parser_previous_token(parser);

  MCC_ASSERT(token.tag == TOKEN_IDENTIFIER);

  const StringView identifier = str_from_token(parser->src, token);

  // TODO: handle typedef
  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);

  // If local variable does not exist
  const IdentifierInfo* variable = lookup_identifier(scope, identifier);
  if (!variable) {
    const StringView error_msg = allocate_printf(
        parser->permanent_arena, "use of undeclared identifier '%.*s'",
        (int)identifier.size, identifier.start);
    parse_error_at(parser, error_msg, token_source_range(token));
  }

  // TODO: handle the case wher variable == nullptr
  *result = (Expr){.tag = EXPR_VARIABLE,
                   .source_range = token_source_range(token),
                   .variable = variable};
  return result;
}

typedef enum Precedence {
  PREC_NONE = 0,
  PREC_ASSIGNMENT,  // =
  PREC_TERNARY,     // ? :
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

static Expr* parse_expr(Parser* parser, struct Scope* scope);
static Expr* parse_group(Parser* parser, struct Scope* scope);
static Expr* parse_unary_op(Parser* parser, struct Scope* scope);
static Expr* parse_binop_left(Parser* parser, Expr* lhs_expr,
                              struct Scope* scope); // left associative
static Expr* parse_assignment(Parser* parser, Expr* lhs_expr,
                              struct Scope* scope); // right associative
static Expr* parse_ternary(Parser* parser, Expr* cond, struct Scope* scope);
static Expr* parse_function_call(Parser* parser, Expr* function,
                                 struct Scope* scope);

typedef Expr* (*PrefixParseFn)(Parser*, struct Scope* scope);
typedef Expr* (*InfixParseFn)(Parser*, Expr*, struct Scope* scope);

typedef struct ParseRule {
  PrefixParseFn prefix;
  InfixParseFn infix;
  Precedence precedence;
} ParseRule;

static ParseRule rules[TOKEN_TYPES_COUNT] = {
    [TOKEN_LEFT_PAREN] = {parse_group, parse_function_call, PREC_CALL},
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
    [TOKEN_QUESTION] = {NULL, parse_ternary, PREC_TERNARY},
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

static ParseRule* get_rule(TokenTag operator_type)
{
  return &rules[operator_type];
}

static Expr* parse_precedence(Parser* parser, Precedence precedence,
                              struct Scope* scope)
{
  parse_advance(parser);

  const Token previous_token = parser_previous_token(parser);

  const PrefixParseFn prefix_rule = get_rule(previous_token.tag)->prefix;
  if (prefix_rule == NULL) {
    parse_panic_at_token(parser, str("Expect valid expression"),
                         previous_token);
    Expr* error_expr = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
    *error_expr = (Expr){
        .tag = EXPR_INVALID,
        .source_range = token_source_range(previous_token),
    };
    return error_expr;
  }

  Expr* expr = prefix_rule(parser, scope);

  while (precedence <= get_rule(parser_current_token(parser).tag)->precedence) {
    parse_advance(parser);
    InfixParseFn infix_rule =
        get_rule(parser_previous_token(parser).tag)->infix;
    expr = infix_rule(parser, expr, scope);
  }

  return expr;
}

static Expr* parse_group(Parser* parser, struct Scope* scope)
{
  Expr* result = parse_expr(parser, scope);
  parse_consume(parser, TOKEN_RIGHT_PAREN, "Expect )");
  return result;
}

static Expr* parse_unary_op(Parser* parser, Scope* scope)
{
  Token operator_token = parser_previous_token(parser);

  UnaryOpType operator_type;
  switch (operator_token.tag) {
  case TOKEN_MINUS: operator_type = UNARY_OP_NEGATION; break;
  case TOKEN_TILDE: operator_type = UNARY_OP_BITWISE_TYPE_COMPLEMENT; break;
  case TOKEN_NOT: operator_type = UNARY_OP_NOT; break;
  default: MCC_UNREACHABLE();
  }

  // Inner expression
  Expr* expr = parse_precedence(parser, PREC_UNARY, scope);

  // TODO: type check

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

static BinaryOpType binop_type_from_token_type(TokenTag token_type)
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
                             enum Associativity associativity, Scope* scope)
{
  Token operator_token = parser_previous_token(parser);

  const TokenTag operator_type = operator_token.tag;
  const ParseRule* rule = get_rule(operator_type);
  Expr* rhs_expr = parse_precedence(
      parser,
      (Precedence)(rule->precedence +
                   ((associativity == ASSOCIATIVITY_LEFT) ? 1 : 0)),
      scope);

  BinaryOpType binary_op_type = binop_type_from_token_type(operator_type);

  // TODO: type check

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

static Expr* parse_binop_left(Parser* parser, Expr* lhs_expr, Scope* scope)
{
  return parse_binary_op(parser, lhs_expr, ASSOCIATIVITY_LEFT, scope);
}

static Expr* parse_assignment(Parser* parser, Expr* lhs_expr, Scope* scope)
{
  // Check whether the left hand side is lvalue
  if (lhs_expr->tag != EXPR_VARIABLE) {
    parse_error_at(parser, str("expression is not assignable"),
                   lhs_expr->source_range);
  }
  return parse_binary_op(parser, lhs_expr, ASSOCIATIVITY_RIGHT, scope);
}

static Expr* parse_ternary(Parser* parser, Expr* cond, struct Scope* scope)
{
  Expr* true_expr = parse_expr(parser, scope);
  parse_consume(parser, TOKEN_COLON, "expect ':'");
  Expr* false_expr = parse_precedence(parser, PREC_TERNARY, scope);

  Expr* result = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *result = (Expr){.tag = EXPR_TERNARY,
                   .source_range = source_range_union(cond->source_range,
                                                      false_expr->source_range),
                   .ternary = (struct TernaryExpr){.cond = cond,
                                                   .true_expr = true_expr,
                                                   .false_expr = false_expr}};
  return result;
}

struct ExprVec {
  size_t length;
  size_t capacity;
  Expr** data;
};

static struct ExprVec parse_arg_list(Parser* parser, Scope* scope)
{
  struct ExprVec args_vec = {};
  if (!token_match_or_eof(parser, TOKEN_RIGHT_PAREN)) {
    // first arg
    DYNARRAY_PUSH_BACK(&args_vec, Expr*, &parser->scratch_arena,
                       parse_expr(parser, scope));

    while (!token_match_or_eof(parser, TOKEN_RIGHT_PAREN)) {
      parse_consume(parser, TOKEN_COMMA, "expect ','");
      DYNARRAY_PUSH_BACK(&args_vec, Expr*, &parser->scratch_arena,
                         parse_expr(parser, scope));
    }
  }

  parse_consume(parser, TOKEN_RIGHT_PAREN,
                "expect ')' at the end of a function call");
  return args_vec;
}

static Expr* parse_function_call(Parser* parser, Expr* function, Scope* scope)
{
  struct ExprVec args_vec = parse_arg_list(parser, scope);
  const uint32_t arg_count = u32_from_usize(args_vec.length);

  Expr** args = ARENA_ALLOC_ARRAY(parser->permanent_arena, Expr*, arg_count);
  if (args_vec.length != 0) {
    memcpy(args, args_vec.data, args_vec.length * sizeof(Expr*));
  }

  Expr* expr = ARENA_ALLOC_OBJECT(parser->permanent_arena, Expr);
  *expr = (Expr){.tag = EXPR_CALL,
                 .source_range = source_range_union(
                     function->source_range,
                     token_source_range(parser_previous_token(parser))),
                 .call = {
                     .function = function,
                     .arg_count = arg_count,
                     .args = args,
                 }};
  return expr;
}

static Expr* parse_expr(Parser* parser, Scope* scope)
{
  return parse_precedence(parser, PREC_ASSIGNMENT, scope);
}
#pragma endregion

#pragma region Statement/declaration parsing

static Stmt parse_stmt(Parser* parser, struct Scope* scope);

static struct ReturnStmt parse_return_stmt(Parser* parser, Scope* scope)
{
  Expr* expr = parse_expr(parser, scope);
  parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  return (struct ReturnStmt){.expr = expr};
}

struct BlockItemVec {
  size_t capacity;
  size_t length;
  BlockItem* data;
};

static void assign_type_if_null(Parser* parser, const Type** target_type,
                                const Type* new_type)
{
  if (*target_type == nullptr) {
    *target_type = new_type;
  } else {
    StringBuffer message = string_buffer_new(parser->permanent_arena);
    string_buffer_append(&message, str("Cannot combine with previous `"));
    format_type_to(&message, *target_type);
    string_buffer_append(&message, str("` type specifier"));
    parse_panic_at_token(parser, str_from_buffer(&message),
                         parser_current_token(parser));
  }
}

static void assign_storage_class_if_none(Parser* parser,
                                         StorageClass* target_class,
                                         StorageClass new_class)
{
  if (*target_class == STORAGE_CLASS_NONE) {
    *target_class = new_class;
  } else {
    parse_panic_at_token(
        parser, str("multiple storage classes in declaration specifiers"),
        parser_current_token(parser));
  }
}

typedef struct DeclSpecifier {
  const Type* type;
  StorageClass storage_class;
} DeclSpecifier;

// Declaration specifiers include types, storage durations, and type
// modifiers. They are converted into a single type and at most one storage
// class specifier.
//
// Note the ANSI-C style defaulting to `int` is not supported. Instead, we
// always want a type
//
// Examples:
// - int
// - static const int
static DeclSpecifier parse_decl_specifiers(Parser* parser)
{
  const Type* type = nullptr;
  StorageClass storage_class = STORAGE_CLASS_NONE;

  while (true) {
    Token current_token = parser_current_token(parser);
    switch (current_token.tag) {
    case TOKEN_KEYWORD_VOID:
      assign_type_if_null(parser, &type, typ_void);
      break;
    case TOKEN_KEYWORD_INT: assign_type_if_null(parser, &type, typ_int); break;
    case TOKEN_KEYWORD_EXTERN:
      assign_storage_class_if_none(parser, &storage_class,
                                   STORAGE_CLASS_EXTERN);
      break;
    case TOKEN_KEYWORD_STATIC:
      assign_storage_class_if_none(parser, &storage_class,
                                   STORAGE_CLASS_STATIC);
      break;
    default: goto done;
    }

    parse_advance(parser);
  }

done:
  if (type == nullptr) {
    parse_panic_at_token(
        parser, str("A type specifier is required for all declarations"),
        parser_current_token(parser));
  }
  return (DeclSpecifier){
      .type = type,
      .storage_class = storage_class,
  };
}

static FunctionDecl* parse_function_decl(Parser* parser,
                                         DeclSpecifier decl_specifier,
                                         Token name_token, struct Scope* scope);

static VariableDecl parse_variable_decl(Parser* parser,
                                        DeclSpecifier decl_specifier,
                                        Token name_token, struct Scope* scope)
{
  const StringView name = str_from_token(parser->src, name_token);
  // TODO: handle different linkages
  IdentifierInfo* variable = add_identifier(
      scope, name, IDENT_OBJECT, LINKAGE_NONE, parser->permanent_arena);
  if (!variable) {
    const StringView error_msg =
        allocate_printf(parser->permanent_arena, "redefinition of '%.*s'",
                        (int)name.size, name.start);
    parse_error_at(parser, error_msg, token_source_range(name_token));
  }

  Expr* initializer = nullptr;
  if (parser_current_token(parser).tag == TOKEN_EQUAL) {
    parse_advance(parser);
    initializer = parse_expr(parser, scope);
  }
  parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");

  // TODO: handle the case where variable == nullptr
  return (VariableDecl){.type = decl_specifier.type,
                        .storage_class = decl_specifier.storage_class,
                        .name = variable,
                        .initializer = initializer};
}

static Decl parse_decl(Parser* parser, struct Scope* scope)
{
  const DeclSpecifier decl_specifier = parse_decl_specifiers(parser);
  const Token name_token = parse_identifier(parser);

  if (parser_current_token(parser).tag == TOKEN_LEFT_PAREN) {
    return (Decl){
        .tag = DECL_FUNC,
        .func = parse_function_decl(parser, decl_specifier, name_token, scope),
    };
  } else {
    return (Decl){
        .tag = DECL_VAR,
        .var = parse_variable_decl(parser, decl_specifier, name_token, scope),
    };
  }
}

// Find the next synchronization token (`}` or `;`)
static void parser_panic_synchronize(Parser* parser)
{
  while (true) {
    const TokenTag current_token_type = parser_current_token(parser).tag;
    if (current_token_type == TOKEN_RIGHT_BRACE ||
        current_token_type == TOKEN_SEMICOLON ||
        current_token_type == TOKEN_EOF) {
      break;
    }
    parser->current_token_index++;
  }
}

// Check whether the current token can be treated as the start of the
// declaration specifier
static bool is_decl_specifier(Parser* parser)
{
  const Token current_token = parser_current_token(parser);
  switch (current_token.tag) {
  case TOKEN_KEYWORD_VOID: [[fallthrough]];
  case TOKEN_KEYWORD_INT: [[fallthrough]];
  case TOKEN_KEYWORD_EXTERN: [[fallthrough]];
  case TOKEN_KEYWORD_STATIC: return true;
  default: return false;
  }
}

static BlockItem parse_block_item(Parser* parser, Scope* scope)
{
  BlockItem result;

  if (is_decl_specifier(parser)) {
    result =
        (BlockItem){.tag = BLOCK_ITEM_DECL, .decl = parse_decl(parser, scope)};
  } else {
    result =
        (BlockItem){.tag = BLOCK_ITEM_STMT, .stmt = parse_stmt(parser, scope)};
  }

  if (parser->in_panic_mode) {
    parser->in_panic_mode = false;
    parser_panic_synchronize(parser);
  }

  return result;
}

static Block parse_block(Parser* parser, Scope* scope)
{
  struct BlockItemVec items_vec = {};

  while (!token_match_or_eof(parser, TOKEN_RIGHT_BRACE)) {
    BlockItem item = parse_block_item(parser, scope);
    DYNARRAY_PUSH_BACK(&items_vec, BlockItem, &parser->scratch_arena, item);
  }
  parse_consume(parser, TOKEN_RIGHT_BRACE, "Expect `}`");

  BlockItem* block_items =
      ARENA_ALLOC_ARRAY(parser->permanent_arena, BlockItem, items_vec.length);
  if (items_vec.length != 0) {
    memcpy(block_items, items_vec.data, items_vec.length * sizeof(BlockItem));
  }

  return (Block){.children = block_items, .child_count = items_vec.length};
}

struct IfStmt parse_if_stmt(Parser* parser, Scope* scope)
{
  parse_consume(parser, TOKEN_LEFT_PAREN, "expect '('");
  Expr* cond = parse_expr(parser, scope);
  parse_consume(parser, TOKEN_RIGHT_PAREN, "expect ')'");

  Stmt* then = ARENA_ALLOC_OBJECT(parser->permanent_arena, Stmt);
  *then = parse_stmt(parser, scope);

  Stmt* els = nullptr;
  if (parser_current_token(parser).tag == TOKEN_KEYWORD_ELSE) {
    parse_advance(parser);

    els = ARENA_ALLOC_OBJECT(parser->permanent_arena, Stmt);
    *els = parse_stmt(parser, scope);
  }

  return (struct IfStmt){
      .cond = cond,
      .then = then,
      .els = els,
  };
}

static Stmt parse_stmt(Parser* parser, Scope* scope)
{
  const Token start_token = parser_current_token(parser);

  Stmt result;

  switch (start_token.tag) {
  case TOKEN_SEMICOLON: {
    parse_advance(parser);

    result = (Stmt){.tag = STMT_EMPTY};
    break;
  }

  case TOKEN_KEYWORD_RETURN: {
    parse_advance(parser);

    struct ReturnStmt return_stmt = parse_return_stmt(parser, scope);
    result = (Stmt){.tag = STMT_RETURN, .ret = return_stmt};
    break;
  }
  case TOKEN_KEYWORD_BREAK: {
    parse_advance(parser);
    parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");

    result = (Stmt){.tag = STMT_BREAK};
    break;
  }
  case TOKEN_KEYWORD_CONTINUE: {
    parse_advance(parser);
    parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");

    result = (Stmt){.tag = STMT_CONTINUE};
    break;
  }
  case TOKEN_LEFT_BRACE: {
    parse_advance(parser);

    Scope* block_scope = new_scope(scope, parser->permanent_arena);
    const Block compound = parse_block(parser, block_scope);
    result = (Stmt){.tag = STMT_COMPOUND, .compound = compound};
    break;
  }
  case TOKEN_KEYWORD_IF: {
    parse_advance(parser);
    const struct IfStmt if_then = parse_if_stmt(parser, scope);

    result = (Stmt){.tag = STMT_IF, .if_then = if_then};
    break;
  }
  case TOKEN_KEYWORD_WHILE: {
    parse_advance(parser);
    parse_consume(parser, TOKEN_LEFT_PAREN, "expect '('");
    Expr* cond = parse_expr(parser, scope);
    parse_consume(parser, TOKEN_RIGHT_PAREN, "expect ')'");

    Stmt* body = ARENA_ALLOC_OBJECT(parser->permanent_arena, Stmt);
    *body = parse_stmt(parser, scope);
    result =
        (Stmt){.tag = STMT_WHILE, .while_loop = {.cond = cond, .body = body}};
    break;
  }
  case TOKEN_KEYWORD_DO: {
    parse_advance(parser);
    Stmt* body = ARENA_ALLOC_OBJECT(parser->permanent_arena, Stmt);
    *body = parse_stmt(parser, scope);
    parse_consume(parser, TOKEN_KEYWORD_WHILE, "expect \"while\"");

    parse_consume(parser, TOKEN_LEFT_PAREN, "expect '('");
    Expr* cond = parse_expr(parser, scope);
    parse_consume(parser, TOKEN_RIGHT_PAREN, "expect ')'");
    parse_consume(parser, TOKEN_SEMICOLON,
                  "expect ';' after do/while statement");
    result = (Stmt){.tag = STMT_DO_WHILE,
                    .while_loop = {.cond = cond, .body = body}};
    break;
  }
  case TOKEN_KEYWORD_FOR: {
    parse_advance(parser);
    parse_consume(parser, TOKEN_LEFT_PAREN, "expect '('");

    // init
    ForInit init = {};
    switch (parser_current_token(parser).tag) {
      // TODO: handle cases other than int
    case TOKEN_KEYWORD_INT: {
      // for loop introduce a new scope
      scope = new_scope(scope, parser->permanent_arena);

      // TODO: fix this
      const DeclSpecifier specifier = {
          .type = typ_int,
          .storage_class = STORAGE_CLASS_NONE,
      };

      parse_advance(parser);
      VariableDecl* decl =
          ARENA_ALLOC_OBJECT(parser->permanent_arena, VariableDecl);

      const Token name_token = parse_identifier(parser);
      *decl = parse_variable_decl(parser, specifier, name_token, scope);
      init = (ForInit){.tag = FOR_INIT_DECL, .decl = decl};
    } break;
    case TOKEN_SEMICOLON: {
      init = (ForInit){.tag = FOR_INIT_EXPR, .expr = nullptr};
      parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");
    } break;
    default: {
      init = (ForInit){.tag = FOR_INIT_EXPR, .expr = parse_expr(parser, scope)};
      parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");
    } break;
    }

    // cond
    Expr* cond = parser_current_token(parser).tag == TOKEN_SEMICOLON
                     ? nullptr
                     : parse_expr(parser, scope);
    parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");

    // post
    Expr* post = parser_current_token(parser).tag == TOKEN_RIGHT_PAREN
                     ? nullptr
                     : parse_expr(parser, scope);
    parse_consume(parser, TOKEN_RIGHT_PAREN, "expect ')'");

    Stmt* body = ARENA_ALLOC_OBJECT(parser->permanent_arena, Stmt);
    *body = parse_stmt(parser, scope);
    result = (Stmt){.tag = STMT_FOR,
                    .for_loop = {
                        .init = init,
                        .cond = cond,
                        .post = post,
                        .body = body,
                    }};

    break;
  }
  default: {
    Expr* expr = parse_expr(parser, scope);
    parse_consume(parser, TOKEN_SEMICOLON, "expect ';'");
    result = (Stmt){.tag = STMT_EXPR, .expr = expr};
    break;
  }
  }

  result.source_range =
      source_range_union(token_source_range(start_token),
                         token_source_range(parser_previous_token(parser)));
  return result;
}

struct ParameterVec {
  uint32_t length;
  uint32_t capacity;
  IdentifierInfo** data;
};

static IdentifierInfo* parse_parameter(Parser* parser, Scope* scope)
{
  const Token current_token = parser_current_token(parser);

  switch (current_token.tag) {
  case TOKEN_KEYWORD_VOID: {
    parse_error_at(
        parser, str("'void' must be the first and only parameter if specified"),
        token_source_range(current_token));
    parse_advance(parser);
  } break;
  case TOKEN_KEYWORD_INT: {
    parse_advance(parser);

    const Token identifier_token = parser_current_token(parser);
    StringView identifier = {};
    if (identifier_token.tag == TOKEN_IDENTIFIER) {
      identifier = str_from_token(parser->src, identifier_token);
      parse_advance(parser);
    }

    IdentifierInfo* name = add_identifier(
        scope, identifier, IDENT_OBJECT, LINKAGE_NONE, parser->permanent_arena);
    // TODO: error handling
    MCC_ASSERT(name != nullptr);
    return name;
  } break;
  default:
    parse_panic_at_token(parser, str("Expect parameter declarator"),
                         current_token);
    parse_advance(parser);
    break;
  }
  // TODO: error handling
  return nullptr;
}

static Parameters parse_parameter_list(Parser* parser, Scope* scope)
{
  parse_consume(parser, TOKEN_LEFT_PAREN, "Expect (");

  if (parser_current_token(parser).tag == TOKEN_KEYWORD_VOID) {
    parse_advance(parser);
    parse_consume(parser, TOKEN_RIGHT_PAREN, "Expect )");

    return (Parameters){};
  }

  if (token_match_or_eof(parser, TOKEN_RIGHT_PAREN)) {
    parse_advance(parser);

    // TODO: warn when the parameter list is empty in pre-C23 mode
    return (Parameters){};
  }

  struct ParameterVec parameters_vec = {};

  {
    // first parameter
    IdentifierInfo* name = parse_parameter(parser, scope);
    DYNARRAY_PUSH_BACK(&parameters_vec, IdentifierInfo*, &parser->scratch_arena,
                       name);
  }
  while (!token_match_or_eof(parser, TOKEN_RIGHT_PAREN)) {
    parse_consume(parser, TOKEN_COMMA, "Expect ','");
    IdentifierInfo* name = parse_parameter(parser, scope);
    DYNARRAY_PUSH_BACK(&parameters_vec, IdentifierInfo*, &parser->scratch_arena,
                       name);
  }

  parse_consume(parser, TOKEN_RIGHT_PAREN, "Expect )");

  IdentifierInfo** params = ARENA_ALLOC_ARRAY(
      parser->permanent_arena, IdentifierInfo*, parameters_vec.length);
  if (parameters_vec.length != 0) {
    memcpy(params, parameters_vec.data,
           parameters_vec.length * sizeof(IdentifierInfo*));
  }

  return (Parameters){
      .length = parameters_vec.length,
      .data = params,
  };
}

static FunctionDecl* parse_function_decl(Parser* parser,
                                         DeclSpecifier decl_specifier,
                                         Token name_token, Scope* scope)
{
  StringView name = str_from_token(parser->src, name_token);
  IdentifierInfo* function_ident = add_identifier(
      scope, name, IDENT_FUNCTION, LINKAGE_EXTERNAL, parser->permanent_arena);

  if (!function_ident) {
    function_ident = lookup_identifier(scope, name);
    MCC_ASSERT(function_ident != nullptr);
    if (function_ident->kind != IDENT_FUNCTION) {
      const StringView error_msg =
          allocate_printf(parser->permanent_arena,
                          "redefinition of '%.*s' as different kind of symbol",
                          (int)name.size, name.start);
      parse_error_at(parser, error_msg, token_source_range(name_token));
    }
  }

  hashmap_try_insert(&parser->functions, name, function_ident,
                     parser->permanent_arena);

  Scope* function_scope =
      new_scope(parser->global_scope, parser->permanent_arena);
  const Parameters parameters = parse_parameter_list(parser, function_scope);

  Block* body = NULL;

  if (parser_current_token(parser).tag == TOKEN_LEFT_BRACE) { // is definition
    parse_advance(parser);
    body = ARENA_ALLOC_OBJECT(parser->permanent_arena, Block);
    *body = parse_block(parser, function_scope);
  } else {
    parse_consume(parser, TOKEN_SEMICOLON, "Expect ;");
  }

  FunctionDecl* decl =
      ARENA_ALLOC_OBJECT(parser->permanent_arena, FunctionDecl);
  *decl = (FunctionDecl){
      .return_type = decl_specifier.type,
      .storage_class = decl_specifier.storage_class,
      .source_range =
          source_range_union(token_source_range(name_token),
                             token_source_range(parser_previous_token(parser))),
      .name = function_ident,
      .params = parameters,
      .body = body,
  };
  return decl;
}
#pragma endregion

struct DeclVec {
  uint32_t length;
  uint32_t capacity;
  Decl* data;
};

static TranslationUnit* parse_translation_unit(Parser* parser)
{
  struct DeclVec decl_vec = {};

  while (parser_current_token(parser).tag != TOKEN_EOF) {
    Decl decl = parse_decl(parser, parser->global_scope);
    DYNARRAY_PUSH_BACK(&decl_vec, Decl, &parser->scratch_arena, decl);
  }

  const uint32_t decl_count = decl_vec.length;
  Decl* decls = ARENA_ALLOC_ARRAY(parser->permanent_arena, Decl, decl_count);
  if (decl_vec.length != 0) {
    memcpy(decls, decl_vec.data, decl_vec.length * sizeof(Decl));
  }

  TranslationUnit* tu =
      ARENA_ALLOC_OBJECT(parser->permanent_arena, TranslationUnit);
  *tu = (TranslationUnit){
      .decl_count = decl_count,
      .decls = decls,
      .global_scope = parser->global_scope,
      .functions = parser->functions,
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
                   .scratch_arena = scratch_arena,
                   .global_scope = new_scope(nullptr, permanent_arena),
                   .functions = (HashMap){}};

  TranslationUnit* tu = parse_translation_unit(&parser);

  const bool has_error = parser.errors.data != NULL;
  const ErrorsView errors = (ErrorsView){
      .length = parser.errors.length,
      .data = parser.errors.data,
  };
  return (ParseResult){.ast = has_error ? NULL : tu, .errors = errors};
}
