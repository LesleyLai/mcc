#include <mcc/ir.h>
#include <string.h>

#include <mcc/ast.h>
#include <mcc/dynarray.h>
#include <mcc/format.h>

/*
 * =============================================================================
 * Convenient "constructors"
 * =============================================================================
 */
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

static IRValue ir_variable(StringView name)
{
  return (IRValue){.typ = IR_VALUE_TYPE_VARIABLE, .variable = name};
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
  case BINARY_OP_AND: MCC_UNIMPLEMENTED();
  case BINARY_OP_OR: MCC_UNIMPLEMENTED();
  case BINARY_OP_EQUAL: return IR_EQUAL;
  case BINARY_OP_NOT_EQUAL: return IR_NOT_EQUAL;
  case BINARY_OP_LESS: return IR_LESS;
  case BINARY_OP_LESS_EQUAL: return IR_LESS_EQUAL;
  case BINARY_OP_GREATER: return IR_GREATER;
  case BINARY_OP_GREATER_EQUAL: return IR_GREATER_EQUAL;
  }
  MCC_UNREACHABLE();
}

typedef struct IRInstructions {
  IRInstruction* data;
  uint32_t length;
  uint32_t capacity;
} IRInstructions;

typedef struct IRGenContext {
  Arena* permanent_arena;
  Arena* scratch_arena;
  IRInstructions instructions;
  int fresh_variable_counter;
} IRGenContext;

static void push_instruction(IRGenContext* context, IRInstruction instruction)
{
  DYNARRAY_PUSH_BACK(&context->instructions, IRInstruction,
                     context->scratch_arena, instruction);
}

static StringView create_fresh_variable_name(IRGenContext* context)
{
  char* variable_name_buffer = allocate_printf(context->permanent_arena, "$%d",
                                               context->fresh_variable_counter);
  ++context->fresh_variable_counter;
  return string_view_from_c_str(variable_name_buffer);
}

static IRValue emit_ir_instructions_from_expr(const Expr* expr,
                                              IRGenContext* context)
{
  switch (expr->type) {
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
  }

  MCC_UNREACHABLE();
}

__attribute__((nonnull)) static IRFunctionDef
generate_ir_function_def(const FunctionDecl* decl, Arena* permanent_arena,
                         Arena scratch_arena)

{
  IRGenContext context = (IRGenContext){.permanent_arena = permanent_arena,
                                        .scratch_arena = &scratch_arena,
                                        .instructions = {},
                                        .fresh_variable_counter = 0};

  for (size_t i = 0; i < decl->body->statement_count; ++i) {
    const Stmt stmt = decl->body->statements[i];
    switch (stmt.type) {
    case STMT_INVALID: MCC_UNREACHABLE();
    case STMT_RETURN: {
      IRValue return_value =
          emit_ir_instructions_from_expr(stmt.ret.expr, &context);

      push_instruction(&context,
                       ir_single_operand_instr(IR_RETURN, return_value));
      break;
    }
    case STMT_COMPOUND: MCC_UNIMPLEMENTED(); break;
    }
    // TODO: handle other kinds of statements
  }

  // allocate and copy instructions to permanent arena
  IRInstruction* instructions = ARENA_ALLOC_ARRAY(
      permanent_arena, IRInstruction, context.instructions.length);
  memcpy(instructions, context.instructions.data,
         context.instructions.length * sizeof(IRInstruction));

  return (IRFunctionDef){.name = decl->name,
                         .instruction_count = context.instructions.length,
                         .instructions = instructions};
}

IRProgram* ir_generate(TranslationUnit* ast, Arena* permanent_arena,
                       Arena scratch_arena)
{
  const size_t ir_function_count = ast->decl_count;
  IRFunctionDef* ir_functions =
      ARENA_ALLOC_ARRAY(permanent_arena, IRFunctionDef, ir_function_count);
  for (size_t i = 0; i < ir_function_count; i++) {
    ir_functions[i] = generate_ir_function_def(&ast->decls[i], permanent_arena,
                                               scratch_arena);
  }

  IRProgram* program = ARENA_ALLOC_OBJECT(permanent_arena, IRProgram);
  *program = (IRProgram){
      .function_count = ir_function_count,
      .functions = ir_functions,
  };

  return program;
}
