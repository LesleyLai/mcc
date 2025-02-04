#include <mcc/ir.h>
#include <string.h>

#include <mcc/ast.h>
#include <mcc/dynarray.h>
#include <mcc/format.h>

#include "../frontend/symbol_table.h"

#define ASSIGNMENTS                                                            \
  case BINARY_OP_ASSIGNMENT:                                                   \
  case BINARY_OP_PLUS_EQUAL:                                                   \
  case BINARY_OP_MINUS_EQUAL:                                                  \
  case BINARY_OP_MULT_EQUAL:                                                   \
  case BINARY_OP_DIVIDE_EQUAL:                                                 \
  case BINARY_OP_MOD_EQUAL:                                                    \
  case BINARY_OP_BITWISE_AND_EQUAL:                                            \
  case BINARY_OP_BITWISE_OR_EQUAL:                                             \
  case BINARY_OP_BITWISE_XOR_EQUAL:                                            \
  case BINARY_OP_SHIFT_LEFT_EQUAL:                                             \
  case BINARY_OP_SHIFT_RIGHT_EQUAL

/*
 * =============================================================================
 * Convenient "constructors"
 * =============================================================================
 */
static IRValue ir_constant(int32_t constant)
{
  return (IRValue){.typ = IR_VALUE_TYPE_CONSTANT, .constant = constant};
}

static IRValue ir_variable(StringView name)
{
  return (IRValue){.typ = IR_VALUE_TYPE_VARIABLE, .variable = name};
}

static IRInstruction ir_single_operand_instr(IRInstructionType typ,
                                             IRValue operand)
{
  return (IRInstruction){.typ = typ, .operand1 = operand};
}

static IRInstruction ir_unary_instr(IRInstructionType typ, IRValue dst,
                                    IRValue src)
{
  return (IRInstruction){.typ = typ, .operand1 = dst, .operand2 = src};
}

static IRInstruction ir_label(StringView label)
{
  return (IRInstruction){.typ = IR_LABEL, .label = label};
}

static IRInstruction ir_jmp(StringView label)
{
  return (IRInstruction){.typ = IR_JMP, .label = label};
}

static IRInstruction ir_br(IRValue cond, StringView if_label,
                           StringView else_label)
{
  return (IRInstruction){.typ = IR_BR,
                         .cond = cond,
                         .if_label = if_label,
                         .else_label = else_label};
}

static IRInstructionType instruction_typ_from_unary_op(UnaryOpType op_type)
{
  switch (op_type) {
  case UNARY_OP_INVALID: MCC_UNREACHABLE();
  case UNARY_OP_NEGATION: return IR_NEG;
  case UNARY_OP_BITWISE_TYPE_COMPLEMENT: return IR_COMPLEMENT;
  case UNARY_OP_NOT: return IR_NOT;
  }
  MCC_UNREACHABLE();
}

static IRInstructionType instruction_typ_from_binary_op(BinaryOpType op_type)
{
  switch (op_type) {
  case BINARY_OP_INVALID: MCC_UNREACHABLE();
  case BINARY_OP_PLUS: return IR_ADD;
  case BINARY_OP_MINUS: return IR_SUB;
  case BINARY_OP_MULT: return IR_MUL;
  case BINARY_OP_DIVIDE: return IR_DIV;
  case BINARY_OP_MOD: return IR_MOD;
  case BINARY_OP_BITWISE_AND: return IR_BITWISE_AND;
  case BINARY_OP_BITWISE_OR: return IR_BITWISE_OR;
  case BINARY_OP_BITWISE_XOR: return IR_BITWISE_XOR;
  case BINARY_OP_SHIFT_LEFT: return IR_SHIFT_LEFT;
  case BINARY_OP_SHIFT_RIGHT: return IR_SHIFT_RIGHT_ARITHMETIC;
  case BINARY_OP_AND: MCC_UNREACHABLE();
  case BINARY_OP_OR: MCC_UNREACHABLE();
  case BINARY_OP_EQUAL: return IR_EQUAL;
  case BINARY_OP_NOT_EQUAL: return IR_NOT_EQUAL;
  case BINARY_OP_LESS: return IR_LESS;
  case BINARY_OP_LESS_EQUAL: return IR_LESS_EQUAL;
  case BINARY_OP_GREATER: return IR_GREATER;
  case BINARY_OP_GREATER_EQUAL:
    return IR_GREATER_EQUAL;
  ASSIGNMENTS:
    MCC_UNREACHABLE();
  }
  MCC_UNREACHABLE();
}

typedef struct IRInstructions {
  IRInstruction* data;
  uint32_t length;
  uint32_t capacity;
} IRInstructions;

struct ErrorVec {
  size_t length;
  size_t capacity;
  Error* data;
};

// Context for the whole translation unit
typedef struct IRGenTUContext {
  Arena* permanent_arena;
  Arena* scratch_arena;

  struct ErrorVec errors;
} IRGenTUContext;

// context only for the a single function
typedef struct IRGenProceduralContext {
  IRGenTUContext* tu_context;
  IRInstructions instructions;
  int fresh_variable_counter;
  int fresh_label_counter;
} IRGenProceduralContext;

static void push_instruction(IRGenProceduralContext* context,
                             IRInstruction instruction)
{
  DYNARRAY_PUSH_BACK(&context->instructions, IRInstruction,
                     context->tu_context->scratch_arena, instruction);
}

static StringView create_fresh_variable_name(IRGenProceduralContext* context)
{
  const StringView variable_name_buffer =
      allocate_printf(context->tu_context->permanent_arena, "$%d",
                      context->fresh_variable_counter);
  ++context->fresh_variable_counter;
  return variable_name_buffer;
}

static StringView create_fresh_label_name(IRGenProceduralContext* context,
                                          const char* name)
{
  const StringView variable_name_buffer =
      allocate_printf(context->tu_context->permanent_arena, "%s_%d", name,
                      context->fresh_label_counter);
  ++context->fresh_label_counter;
  return variable_name_buffer;
}

static IRValue emit_ir_instructions_from_expr(const Expr* expr,
                                              IRGenProceduralContext* context);

static bool is_assignment(BinaryOpType typ)
{
  switch (typ) {
  ASSIGNMENTS:
    return true;
  default: return false;
  }
}

static IRValue
emit_ir_instructions_from_binary_expr(const Expr* expr,
                                      IRGenProceduralContext* context)
{
  if (is_assignment(expr->binary_op.binary_op_type)) {
    IRInstructionType compound_op_type = IR_INVALID;
    switch (expr->binary_op.binary_op_type) {
    case BINARY_OP_ASSIGNMENT: break;
    case BINARY_OP_PLUS_EQUAL: compound_op_type = IR_ADD; break;
    case BINARY_OP_MINUS_EQUAL: compound_op_type = IR_SUB; break;
    case BINARY_OP_MULT_EQUAL: compound_op_type = IR_MUL; break;
    case BINARY_OP_DIVIDE_EQUAL: compound_op_type = IR_DIV; break;
    case BINARY_OP_MOD_EQUAL: compound_op_type = IR_MOD; break;
    case BINARY_OP_BITWISE_AND_EQUAL: compound_op_type = IR_BITWISE_AND; break;
    case BINARY_OP_BITWISE_OR_EQUAL: compound_op_type = IR_BITWISE_OR; break;
    case BINARY_OP_BITWISE_XOR_EQUAL: compound_op_type = IR_BITWISE_XOR; break;
    case BINARY_OP_SHIFT_LEFT_EQUAL: compound_op_type = IR_SHIFT_LEFT; break;
    case BINARY_OP_SHIFT_RIGHT_EQUAL:
      compound_op_type = IR_SHIFT_RIGHT_ARITHMETIC;
      break;
    default: MCC_UNREACHABLE();
    }

    const IRValue lhs =
        emit_ir_instructions_from_expr(expr->binary_op.lhs, context);

    const IRValue rhs =
        emit_ir_instructions_from_expr(expr->binary_op.rhs, context);

    if (compound_op_type != IR_INVALID) {
      push_instruction(context, (IRInstruction){
                                    .typ = compound_op_type,
                                    .operand1 = lhs,
                                    .operand2 = lhs,
                                    .operand3 = rhs,
                                });
    } else {
      push_instruction(
          context,
          (IRInstruction){.typ = IR_COPY, .operand1 = lhs, .operand2 = rhs});
    }

    return lhs;
  }

  const IRInstructionType instruction_type =
      instruction_typ_from_binary_op(expr->binary_op.binary_op_type);

  const IRValue lhs =
      emit_ir_instructions_from_expr(expr->binary_op.lhs, context);

  const IRValue rhs =
      emit_ir_instructions_from_expr(expr->binary_op.rhs, context);

  const StringView dst_name = create_fresh_variable_name(context);
  const IRValue dst = ir_variable(dst_name);

  push_instruction(context, (IRInstruction){
                                .typ = instruction_type,
                                .operand1 = dst,
                                .operand2 = lhs,
                                .operand3 = rhs,
                            });

  return dst;
}

static IRValue
emit_ir_instructions_from_logical_and(const Expr* expr,
                                      IRGenProceduralContext* context)
{
  const IRValue lhs =
      emit_ir_instructions_from_expr(expr->binary_op.lhs, context);
  const StringView lhs_true_label =
      create_fresh_label_name(context, "and_lhs_true");
  const StringView rhs_true_label =
      create_fresh_label_name(context, "and_rhs_true");
  const StringView false_label = create_fresh_label_name(context, "and_false");
  const StringView end_label = create_fresh_label_name(context, "and_end");

  const IRValue result = ir_variable(create_fresh_variable_name(context));

  // br lhs .and_lhs_true .and_false
  push_instruction(context, ir_br(lhs, lhs_true_label, false_label));
  // .and_lhs_true:
  push_instruction(context, ir_label(lhs_true_label));

  const IRValue rhs =
      emit_ir_instructions_from_expr(expr->binary_op.rhs, context);

  // br rhs .and_rhs_true .and_false
  push_instruction(context, ir_br(rhs, rhs_true_label, false_label));

  // .and_rhs_true:
  // result = 1
  // jmp .and_end
  push_instruction(context, ir_label(rhs_true_label));
  push_instruction(context, ir_unary_instr(IR_COPY, result, ir_constant(1)));
  push_instruction(context, ir_jmp(end_label));

  // .and_false:
  // result = 0
  push_instruction(context, ir_label(false_label));
  push_instruction(context, ir_unary_instr(IR_COPY, result, ir_constant(0)));

  // .and_end
  push_instruction(context, ir_label(end_label));

  return result;
}

static IRValue
emit_ir_instructions_from_logical_or(const Expr* expr,
                                     IRGenProceduralContext* context)
{
  const IRValue lhs =
      emit_ir_instructions_from_expr(expr->binary_op.lhs, context);
  const StringView lhs_false_label =
      create_fresh_label_name(context, "or_lhs_false");
  const StringView rhs_false_label =
      create_fresh_label_name(context, "or_rhs_false");
  const StringView true_label = create_fresh_label_name(context, "or_true");
  const StringView end_label = create_fresh_label_name(context, "or_end");

  const IRValue result = ir_variable(create_fresh_variable_name(context));

  // br lhs .or_true .or_lhs_false
  push_instruction(context, ir_br(lhs, true_label, lhs_false_label));
  // .or_lhs_false:
  push_instruction(context, ir_label(lhs_false_label));

  const IRValue rhs =
      emit_ir_instructions_from_expr(expr->binary_op.rhs, context);

  // br rhs .or_true .or_rhs_false
  push_instruction(context, ir_br(rhs, true_label, rhs_false_label));

  // .or_rhs_false:
  // result = 0
  // jmp .or_end
  push_instruction(context, ir_label(rhs_false_label));
  push_instruction(context, ir_unary_instr(IR_COPY, result, ir_constant(0)));
  push_instruction(context, ir_jmp(end_label));

  // .or_true:
  // result = 1
  push_instruction(context, ir_label(true_label));
  push_instruction(context, ir_unary_instr(IR_COPY, result, ir_constant(1)));

  // .or_end:
  push_instruction(context, ir_label(end_label));

  return result;
}

static IRValue emit_ir_instructions_from_expr(const Expr* expr,
                                              IRGenProceduralContext* context)
{
  switch (expr->tag) {
  case EXPR_INVALID: MCC_UNREACHABLE();
  case EXPR_CONST:
    return (IRValue){.typ = IR_VALUE_TYPE_CONSTANT,
                     .constant = expr->const_expr.val};
  case EXPR_UNARY: {
    const IRValue src =
        emit_ir_instructions_from_expr(expr->unary_op.inner_expr, context);

    const StringView dst_name = create_fresh_variable_name(context);
    const IRValue dst = ir_variable(dst_name);

    const IRInstructionType instruction_type =
        instruction_typ_from_unary_op(expr->unary_op.unary_op_type);

    push_instruction(context, ir_unary_instr(instruction_type, dst, src));

    return dst;
  }
  case EXPR_BINARY: {
    switch (expr->binary_op.binary_op_type) {
    case BINARY_OP_AND:
      return emit_ir_instructions_from_logical_and(expr, context);
    case BINARY_OP_OR:
      return emit_ir_instructions_from_logical_or(expr, context);
    default: return emit_ir_instructions_from_binary_expr(expr, context);
    }
  }
  case EXPR_VARIABLE: return ir_variable(expr->variable->rewrote_name);
  case EXPR_TERNARY: {
    const StringView true_label =
        create_fresh_label_name(context, "ternary_true");
    const StringView false_label =
        create_fresh_label_name(context, "ternary_false");
    const StringView end_label =
        create_fresh_label_name(context, "ternary_end");

    const IRValue result = ir_variable(create_fresh_variable_name(context));

    const IRValue cond =
        emit_ir_instructions_from_expr(expr->ternary.cond, context);

    // br cond .ternary_true .ternary_false
    push_instruction(context, ir_br(cond, true_label, false_label));

    // .ternary_true
    // result = {{ true branch }}
    // jmp .ternary_end
    push_instruction(context, ir_label(true_label));
    const IRValue true_value =
        emit_ir_instructions_from_expr(expr->ternary.true_expr, context);
    push_instruction(context, ir_unary_instr(IR_COPY, result, true_value));
    push_instruction(context, ir_jmp(end_label));

    // .ternary_false
    // result = {{ false branch }}
    push_instruction(context, ir_label(false_label));
    const IRValue false_value =
        emit_ir_instructions_from_expr(expr->ternary.false_expr, context);
    push_instruction(context, ir_unary_instr(IR_COPY, result, false_value));

    // .ternary_end
    push_instruction(context, ir_label(end_label));
    return result;
  }
  case EXPR_CALL: {
    Expr* function = expr->call.function;
    MCC_ASSERT(function->tag == EXPR_VARIABLE);
    const StringView function_name = function->variable->name;

    uint32_t arg_count = expr->call.arg_count;
    IRValue* args = ARENA_ALLOC_ARRAY(context->tu_context->permanent_arena,
                                      IRValue, arg_count);
    for (uint32_t i = 0; i < arg_count; ++i) {
      args[i] = emit_ir_instructions_from_expr(expr->call.args[i], context);
    }

    const IRValue result = ir_variable(create_fresh_variable_name(context));
    push_instruction(context, (IRInstruction){.typ = IR_CALL,
                                              .call = {
                                                  .func_name = function_name,
                                                  .dest = result,
                                                  .arg_count = arg_count,
                                                  .args = args,
                                              }});

    return result;
  }
  }

  MCC_UNREACHABLE();
}

typedef struct BreakContinueInfo {
  StringView break_label;
  StringView continue_label;
} BreakContinueInfo;

static void emit_ir_instructions_from_stmt(const Stmt* stmt,
                                           IRGenProceduralContext* context,
                                           BreakContinueInfo* break_info);

static void emit_ir_instructions_from_decl(const VariableDecl* decl,
                                           IRGenProceduralContext* context)
{
  if (decl->initializer != nullptr) {
    const IRValue value =
        emit_ir_instructions_from_expr(decl->initializer, context);
    push_instruction(
        context,
        ir_unary_instr(IR_COPY, ir_variable(decl->name->rewrote_name), value));
  }
}

static void
emit_ir_instructions_from_block_item(const BlockItem* item,
                                     IRGenProceduralContext* context,
                                     BreakContinueInfo* break_info)
{
  switch (item->tag) {
  case BLOCK_ITEM_STMT: {
    const Stmt stmt = item->stmt;
    emit_ir_instructions_from_stmt(&stmt, context, break_info);
  } break;
  case BLOCK_ITEM_DECL:
    switch (item->decl.tag) {
    case DECL_INVALID: MCC_UNREACHABLE(); break;
    case DECL_VAR:
      emit_ir_instructions_from_decl(&item->decl.var, context);
      break;
    case DECL_FUNC:
      if (item->decl.func->body != nullptr) {
        Error error =
            (Error){.msg = str("nested function definition is not permitted"),
                    .range = item->decl.func->source_range};
        DYNARRAY_PUSH_BACK(&context->tu_context->errors, Error,
                           context->tu_context->permanent_arena, error);
      }
      break;
    }
    break;
  }
}

static void emit_ir_instructions_from_for_init(ForInit init,
                                               IRGenProceduralContext* context)
{
  switch (init.tag) {
  case FOR_INIT_INVALID: MCC_UNREACHABLE();
  case FOR_INIT_DECL: emit_ir_instructions_from_decl(init.decl, context); break;
  case FOR_INIT_EXPR:
    if (init.expr) { emit_ir_instructions_from_expr(init.expr, context); }
    break;
  }
}

static void emit_ir_instructions_from_stmt(const Stmt* stmt,
                                           IRGenProceduralContext* context,
                                           BreakContinueInfo* break_info)
{
  switch (stmt->tag) {
  case STMT_INVALID: MCC_UNREACHABLE();
  case STMT_EMPTY: break;
  case STMT_EXPR: {
    emit_ir_instructions_from_expr(stmt->expr, context);
  } break;
  case STMT_RETURN: {
    const IRValue return_value =
        emit_ir_instructions_from_expr(stmt->ret.expr, context);

    push_instruction(context, ir_single_operand_instr(IR_RETURN, return_value));
  } break;
  case STMT_COMPOUND: {
    for (size_t i = 0; i < stmt->compound.child_count; ++i) {
      emit_ir_instructions_from_block_item(&stmt->compound.children[i], context,
                                           break_info);
    }
  } break;
  case STMT_IF: {
    const StringView if_label = create_fresh_label_name(context, "if");
    const StringView if_end_label = create_fresh_label_name(context, "if_end");

    const IRValue cond =
        emit_ir_instructions_from_expr(stmt->if_then.cond, context);

    if (stmt->if_then.els == nullptr) {
      // br cond .if .if_end
      push_instruction(context, ir_br(cond, if_label, if_end_label));

      // .if
      // {{ then branch }}
      push_instruction(context, ir_label(if_label));
      emit_ir_instructions_from_stmt(stmt->if_then.then, context, break_info);
    } else {
      const StringView else_label = create_fresh_label_name(context, "else");

      // br cond .if .else
      push_instruction(context, ir_br(cond, if_label, else_label));

      // .if
      // {{ then branch }}
      // jmp .end
      push_instruction(context, ir_label(if_label));
      emit_ir_instructions_from_stmt(stmt->if_then.then, context, break_info);
      push_instruction(context, ir_jmp(if_end_label));

      // .else
      // {{ else branch }}
      push_instruction(context, ir_label(else_label));
      emit_ir_instructions_from_stmt(stmt->if_then.els, context, break_info);
    }

    // .if_end
    push_instruction(context, ir_label(if_end_label));
  } break;
  case STMT_WHILE: {
    const StringView start_label =
        create_fresh_label_name(context, "while_start");
    const StringView body_label =
        create_fresh_label_name(context, "while_body");
    const StringView end_label = create_fresh_label_name(context, "while_end");

    // .start
    push_instruction(context, ir_label(start_label));

    // cond = {{ evaluate condition }}
    const IRValue cond =
        emit_ir_instructions_from_expr(stmt->while_loop.cond, context);

    // br cond .body .end
    push_instruction(context, ir_br(cond, body_label, end_label));

    // .body
    // {{ execute body }}
    push_instruction(context, ir_label(body_label));
    struct BreakContinueInfo loop_info = (BreakContinueInfo){
        .break_label = end_label,
        .continue_label = start_label,
    };
    emit_ir_instructions_from_stmt(stmt->while_loop.body, context, &loop_info);

    // jmp .start
    push_instruction(context, ir_jmp(start_label));

    // .end
    push_instruction(context, ir_label(end_label));

  } break;
  case STMT_DO_WHILE: {
    const StringView start_label = create_fresh_label_name(context, "do_start");
    const StringView continue_label =
        create_fresh_label_name(context, "do_continue");
    const StringView end_label = create_fresh_label_name(context, "do_end");

    // .start
    push_instruction(context, ir_label(start_label));

    // {{ execute body }}
    struct BreakContinueInfo loop_info = (BreakContinueInfo){
        .break_label = end_label,
        .continue_label = continue_label,
    };
    emit_ir_instructions_from_stmt(stmt->while_loop.body, context, &loop_info);

    // .continue
    // cond = {{ evaluate condition }}
    push_instruction(context, ir_label(continue_label));
    const IRValue cond =
        emit_ir_instructions_from_expr(stmt->while_loop.cond, context);

    // br cond .start .end
    push_instruction(context, ir_br(cond, start_label, end_label));

    // .end
    push_instruction(context, ir_label(end_label));
  } break;
  case STMT_FOR: {
    const StringView start_label =
        create_fresh_label_name(context, "for_start");
    const StringView body_label = create_fresh_label_name(context, "for_body");
    const StringView continue_label =
        create_fresh_label_name(context, "for_continue");
    const StringView end_label = create_fresh_label_name(context, "for_end");

    // {{ for init }}
    emit_ir_instructions_from_for_init(stmt->for_loop.init, context);

    // .start
    push_instruction(context, ir_label(start_label));

    if (stmt->for_loop.cond) {

      // cond = {{ evaluate condition }}
      const IRValue cond =
          emit_ir_instructions_from_expr(stmt->for_loop.cond, context);

      // br cond .body .end
      push_instruction(context, ir_br(cond, body_label, end_label));
    }

    // .body
    // {{ execute body }}
    push_instruction(context, ir_label(body_label));
    struct BreakContinueInfo loop_info = (BreakContinueInfo){
        .break_label = end_label,
        .continue_label = continue_label,
    };
    emit_ir_instructions_from_stmt(stmt->for_loop.body, context, &loop_info);

    // .continue
    push_instruction(context, ir_label(continue_label));

    if (stmt->for_loop.post) {
      // {{ execute post expression }}
      emit_ir_instructions_from_expr(stmt->for_loop.post, context);
    }

    // jmp .start
    push_instruction(context, ir_jmp(start_label));

    // .end
    push_instruction(context, ir_label(end_label));

  } break;
  case STMT_BREAK:
    if (break_info != nullptr) {
      push_instruction(context, ir_jmp(break_info->break_label));
    } else {
      Error error = (Error){
          .msg = str("'break' statement not in loop or switch statement"),
          .range = stmt->source_range};
      DYNARRAY_PUSH_BACK(&context->tu_context->errors, Error,
                         context->tu_context->permanent_arena, error);
    }

    break;
  case STMT_CONTINUE:
    if (break_info != nullptr) {
      push_instruction(context, ir_jmp(break_info->continue_label));
    } else {
      Error error = (Error){
          .msg = str("'continue' statement not in loop or switch statement"),
          .range = stmt->source_range};
      DYNARRAY_PUSH_BACK(&context->tu_context->errors, Error,
                         context->tu_context->permanent_arena, error);
    }
    break;
  }
}

static IRFunctionDef generate_ir_function_def(const FunctionDecl* decl,
                                              IRGenTUContext* tu_context)

{
  IRGenProceduralContext context =
      (IRGenProceduralContext){.tu_context = tu_context,
                               .instructions = {},
                               .fresh_variable_counter = 0};

  for (size_t i = 0; i < decl->body->child_count; ++i) {
    emit_ir_instructions_from_block_item(&decl->body->children[i], &context,
                                         nullptr);
  }

  // return 0 for main if there is no return statement at the end
  // TODO: should only do that for the main function
  if (context.instructions.length == 0 ||
      context.instructions.data[context.instructions.length - 1].typ !=
          IR_RETURN) {
    push_instruction(&context,
                     ir_single_operand_instr(IR_RETURN, ir_constant(0)));
  }

  // allocate and copy instructions to permanent arena
  IRInstruction* instructions = nullptr;
  if (context.instructions.data != nullptr) {
    instructions = ARENA_ALLOC_ARRAY(tu_context->permanent_arena, IRInstruction,
                                     context.instructions.length);
    memcpy(instructions, context.instructions.data,
           context.instructions.length * sizeof(IRInstruction));
  }

  uint32_t param_count = decl->params.length;
  StringView* parameters =
      ARENA_ALLOC_ARRAY(tu_context->permanent_arena, StringView, param_count);
  for (uint32_t i = 0; i < param_count; ++i) {
    parameters[i] = decl->params.data[i]->rewrote_name;
  }

  return (IRFunctionDef){.name = decl->name->name,
                         .param_count = param_count,
                         .params = parameters,
                         .instruction_count = context.instructions.length,
                         .instructions = instructions};
}

typedef struct IRFunctionVec {
  IRFunctionDef* data;
  uint32_t length;
  uint32_t capacity;
} IRFunctionVec;

IRGenerationResult ir_generate(const TranslationUnit* ast,
                               Arena* permanent_arena, Arena scratch_arena)
{
  IRFunctionVec ir_function_vec = {};

  IRGenTUContext context = (IRGenTUContext){.permanent_arena = permanent_arena,
                                            .scratch_arena = &scratch_arena,
                                            .errors = (struct ErrorVec){}};

  for (size_t i = 0; i < ast->decl_count; i++) {
    if (ast->decls[i]->body != nullptr) {
      IRFunctionDef function_def =
          generate_ir_function_def(ast->decls[i], &context);
      DYNARRAY_PUSH_BACK(&ir_function_vec, IRFunctionDef, &scratch_arena,
                         function_def);
    }
  }

  IRFunctionDef* ir_functions =
      ARENA_ALLOC_ARRAY(permanent_arena, IRFunctionDef, ir_function_vec.length);
  memcpy(ir_functions, ir_function_vec.data,
         ir_function_vec.length * sizeof(IRFunctionDef));

  IRProgram* program = nullptr;
  if (context.errors.length == 0) {
    program = ARENA_ALLOC_OBJECT(permanent_arena, IRProgram);
    *program = (IRProgram){
        .function_count = ir_function_vec.length,
        .functions = ir_functions,
    };
  }

  return (IRGenerationResult){.errors =
                                  (ErrorsView){
                                      .length = context.errors.length,
                                      .data = context.errors.data,
                                  },
                              .program = program};
}
