#include <mcc/ast.h>
#include <mcc/dynarray.h>
#include <mcc/format.h>
#include <mcc/sema.h>
#include <mcc/type.h>

#include "symbol_table.h"

// All the type checking functions in this file return `false` to indicate
// encountering an error, which is used to skip further checks

struct ErrorVec {
  size_t length;
  size_t capacity;
  Error* data;
};

typedef struct Context {
  struct ErrorVec errors;
  Arena* permanent_arena;
  Scope* global_scope;
} Context;

#pragma region error reporter
static void error_at(StringView msg, SourceRange range, Context* context)
{
  Error error = (Error){.msg = msg, .range = range};
  DYNARRAY_PUSH_BACK(&context->errors, Error, context->permanent_arena, error);
}

static void report_invalid_unary_args(const Expr* expr, Context* context)
{
  StringBuffer buffer = string_buffer_new(context->permanent_arena);
  string_buffer_append(&buffer, str("invalid argument type '"));
  format_type_to(&buffer, expr->unary_op.inner_expr->type);
  string_buffer_append(&buffer, str("' to unary expression"));
  error_at(str_from_buffer(&buffer), expr->unary_op.inner_expr->source_range,
           context);
}

static void report_invalid_binary_args(const Expr* expr, Context* context)
{
  StringBuffer buffer = string_buffer_new(context->permanent_arena);
  string_buffer_append(&buffer,
                       str("invalid operands to binary expression ('"));
  format_type_to(&buffer, expr->binary_op.lhs->type);
  string_buffer_append(&buffer, str("' and '"));
  format_type_to(&buffer, expr->binary_op.rhs->type);
  string_buffer_append(&buffer, str("')"));
  error_at(str_from_buffer(&buffer), expr->source_range, context);
}

static void report_incompatible_return(const Expr* expr, Context* context)
{
  StringBuffer buffer = string_buffer_new(context->permanent_arena);
  string_buffer_append(&buffer, str("returning '"));
  format_type_to(&buffer, expr->type);
  string_buffer_append(
      &buffer, str("' from a function with incompatible result type 'int'"));
  error_at(str_from_buffer(&buffer), expr->source_range, context);
}

static void report_calling_noncallable(const Expr* function, Context* context)
{
  StringBuffer buffer = string_buffer_new(context->permanent_arena);
  string_buffer_append(&buffer, str("called object with type '"));
  format_type_to(&buffer, function->type);
  string_buffer_append(&buffer, str("', which is not callable"));
  error_at(str_from_buffer(&buffer), function->source_range, context);
}

static void report_arg_count_mismatch(const Expr* function,
                                      uint32_t param_count, uint32_t arg_count,
                                      Context* context)
{
  const StringView msg = allocate_printf(
      context->permanent_arena,
      "too %s arguments to function call, expected %u, have %u",
      param_count > arg_count ? "few" : "many", param_count, arg_count);
  error_at(msg, function->source_range, context);
}

static void report_wrong_arg_type(const Expr* arg, Context* context)
{
  StringBuffer buffer = string_buffer_new(context->permanent_arena);
  string_buffer_append(&buffer, str("passing '"));
  format_type_to(&buffer, arg->type);
  string_buffer_append(&buffer, str("' to parameter of type 'int'"));
  error_at(str_from_buffer(&buffer), arg->source_range, context);
}

static void report_incompatible_initialization(const Expr* initializer,
                                               Context* context)
{
  StringBuffer buffer = string_buffer_new(context->permanent_arena);
  string_buffer_append(&buffer, str("initialization of 'int' from '"));
  format_type_to(&buffer, initializer->type);
  string_buffer_append(&buffer, str("'"));
  error_at(str_from_buffer(&buffer), initializer->source_range, context);
}

static void report_conflicting_decl_type(FunctionDecl* decl, Context* context)
{
  StringView msg =
      allocate_printf(context->permanent_arena, "conflicting types for '%.*s'",
                      (int)decl->name->name.size, decl->name->name.start);
  error_at(msg, decl->source_range, context);
}

static void report_multiple_definition(FunctionDecl* decl, Context* context)
{
  StringView msg =
      allocate_printf(context->permanent_arena, "multiple definition of '%.*s'",
                      (int)decl->name->name.size, decl->name->name.start);
  error_at(msg, decl->source_range, context);
}
#pragma endregion

[[nodiscard]]
static bool type_check_expr(Expr* expr, Context* context);

[[nodiscard]]
static bool type_check_function_call(Expr* function_call, Context* context)
{
  MCC_ASSERT(function_call->tag == EXPR_CALL);

  Expr* function_expr = function_call->call.function;
  if (!type_check_expr(function_expr, context)) { return false; }

  if (function_expr->type->tag != TYPE_FUNCTION) {
    MCC_ASSERT(function_expr->type != nullptr);
    report_calling_noncallable(function_expr, context);
    return false;
  }

  const FunctionType* function_type = (const FunctionType*)function_expr->type;

  const uint32_t arg_count = function_call->call.arg_count;
  if (function_type->param_count != arg_count) {
    report_arg_count_mismatch(function_expr, function_type->param_count,
                              arg_count, context);
    return false;
  }

  for (uint32_t i = 0; i < arg_count; ++i) {
    Expr* arg = function_call->call.args[i];
    if (!type_check_expr(arg, context)) { return false; }

    if (arg->type->tag != TYPE_INTEGER) {
      report_wrong_arg_type(arg, context);
      return false;
    }
  }

  function_call->type = typ_int;
  return true;
}

[[nodiscard]]
static bool type_check_expr(Expr* expr, Context* context)
{
  switch (expr->tag) {
  case EXPR_INVALID: MCC_UNREACHABLE(); break;
  case EXPR_CONST: expr->type = typ_int; return true;
  case EXPR_VARIABLE:
    MCC_ASSERT(expr->variable->type != nullptr);
    expr->type = expr->variable->type;
    return true;
  case EXPR_UNARY:
    if (!type_check_expr(expr->unary_op.inner_expr, context)) { return false; }
    if (expr->unary_op.inner_expr->type->tag != TYPE_INTEGER) {
      report_invalid_unary_args(expr, context);
      return false;
    }
    expr->type = expr->unary_op.inner_expr->type;
    return true;
  case EXPR_BINARY:
    if (!type_check_expr(expr->binary_op.lhs, context) ||
        !type_check_expr(expr->binary_op.rhs, context)) {
      return false;
    }

    if (expr->binary_op.lhs->type->tag != TYPE_INTEGER ||
        expr->binary_op.rhs->type->tag != TYPE_INTEGER) {
      report_invalid_binary_args(expr, context);
      return false;
    }

    expr->type = typ_int;
    return true;
  case EXPR_TERNARY:
    if (!type_check_expr(expr->ternary.cond, context) ||
        !type_check_expr(expr->ternary.false_expr, context) ||
        !type_check_expr(expr->ternary.true_expr, context)) {
      MCC_ASSERT(expr->ternary.cond->type->tag == TYPE_INTEGER);
      // TODO: check the two ternary branches has the same type
      return false;
    }

    MCC_ASSERT(expr->ternary.cond->type->tag == TYPE_INTEGER);
    // TODO: check the two branches has the same type
    expr->type = expr->ternary.true_expr->type;
    return true;
  case EXPR_CALL: return type_check_function_call(expr, context);
  }
  MCC_UNREACHABLE();
}

static bool type_check_block(Block* block, Context* context);

[[nodiscard]] static bool type_check_variable_decl(VariableDecl* decl,
                                                   Context* context);

[[nodiscard]]
static bool type_check_stmt(Stmt* stmt, Context* context)
{
  switch (stmt->tag) {
  case STMT_INVALID: MCC_UNREACHABLE();
  case STMT_EMPTY: return true;
  case STMT_EXPR: return type_check_expr(stmt->expr, context);
  case STMT_COMPOUND: return type_check_block(&stmt->compound, context);
  case STMT_RETURN: {
    // TODO: check it return the expect function return type
    Expr* expr = stmt->ret.expr;
    if (!type_check_expr(expr, context)) { return false; }

    if (expr->type->tag != TYPE_INTEGER) {
      report_incompatible_return(expr, context);
      return false;
    }
    return true;
  }
  case STMT_IF: {
    Expr* cond = stmt->if_then.cond;
    if (!type_check_expr(cond, context)) { return false; }
    MCC_ASSERT(cond->type->tag == TYPE_INTEGER);
    bool result = type_check_stmt(stmt->if_then.then, context);
    if (stmt->if_then.els != nullptr) {
      result &= type_check_stmt(stmt->if_then.els, context);
    }
    return result;
  }
  case STMT_WHILE: [[fallthrough]];
  case STMT_DO_WHILE: {
    Expr* cond = stmt->while_loop.cond;
    if (!type_check_expr(cond, context)) { return false; }
    MCC_ASSERT(cond->type->tag == TYPE_INTEGER);
    return type_check_stmt(stmt->while_loop.body, context);
  }
  case STMT_FOR: {
    ForInit init = stmt->for_loop.init;
    Expr* cond = stmt->for_loop.cond;
    Stmt* body = stmt->for_loop.body;
    Expr* post = stmt->for_loop.post;

    bool result = true;
    switch (init.tag) {
    case FOR_INIT_INVALID: MCC_UNREACHABLE();
    case FOR_INIT_DECL:
      if (!type_check_variable_decl(init.decl, context)) { return false; }
      break;
    case FOR_INIT_EXPR: {
      if (init.expr) { result &= type_check_expr(init.expr, context); }
    } break;
    }

    if (cond) { result &= type_check_expr(cond, context); }
    result &= type_check_stmt(body, context);
    if (post) { result &= type_check_expr(post, context); }
    if (!result) { return result; }

    return true;
  }
  case STMT_BREAK:
  case STMT_CONTINUE: return true;
  }
  MCC_UNREACHABLE();
}

[[nodiscard]] static bool type_check_variable_decl(VariableDecl* decl,
                                                   Context* context)
{
  decl->name->type = typ_int;

  if (decl->initializer) {
    // TODO: handle redefinition of static/global variables
    MCC_ASSERT(decl->name->has_definition == false);

    decl->name->has_definition = true;

    if (!type_check_expr(decl->initializer, context)) { return false; }

    if (decl->initializer->type->tag != TYPE_INTEGER) {
      report_incompatible_initialization(decl->initializer, context);
      return false;
    }
  }
  return true;
}

static bool type_check_function_decl(FunctionDecl* decl, Context* context);

static bool type_check_block(Block* block, Context* context)
{
  bool result = true;
  for (uint32_t i = 0; i < block->child_count; ++i) {
    BlockItem* item = &block->children[i];
    switch (item->tag) {
    case BLOCK_ITEM_STMT:
      result &= type_check_stmt(&item->stmt, context);
      break;
    case BLOCK_ITEM_DECL:
      switch (item->decl.tag) {
      case DECL_INVALID: MCC_UNREACHABLE(); break;
      case DECL_VAR:
        if (!type_check_variable_decl(&item->decl.var, context)) {
          return false;
        }
        break;
      case DECL_FUNC:
        MCC_ASSERT(item->decl.func != nullptr);
        if (!type_check_function_decl(item->decl.func, context)) {
          return false;
        }
        break;
      }
      break;
    }
  }
  return result;
}

static bool type_check_function_decl(FunctionDecl* decl, Context* context)
{
  StringView function_name = decl->name->name;
  Identifier* function_ident = hashmap_lookup(&functions, function_name);

  if (function_ident->type == nullptr) {
    function_ident->type =
        func_type(typ_int, decl->params.length, context->permanent_arena);
  } else {
    MCC_ASSERT(function_ident->type->tag == TYPE_FUNCTION);
    const FunctionType* function_type = (FunctionType*)function_ident->type;
    if (function_type->return_type != typ_int ||
        function_type->param_count != decl->params.length) {
      report_conflicting_decl_type(decl, context);
      return false;
    }
  }

  if (decl->body != nullptr) {
    if (function_ident->has_definition) {
      report_multiple_definition(decl, context);
      return false;
    }
    function_ident->has_definition = true;

    for (uint32_t i = 0; i < decl->params.length; ++i) {
      Identifier* param = decl->params.data[i];
      param->type = typ_int;
    }
    if (!type_check_block(decl->body, context)) { return false; }
  }
  return true;
}

ErrorsView type_check(TranslationUnit* ast, Arena* permanent_arena)
{
  Context context = {.permanent_arena = permanent_arena,
                     .global_scope = ast->global_scope};

  for (uint32_t i = 0; i < ast->decl_count; ++i) {
    type_check_function_decl(ast->decls[i], &context);
  }

  return (ErrorsView){
      .data = context.errors.data,
      .length = context.errors.length,
  };
}
