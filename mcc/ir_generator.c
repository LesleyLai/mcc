#include "ir.h"
#include "utils/format.h"

static IRInstructionType instruction_typ_from_unary_op(UnaryOpType op_type)
{
  switch (op_type) {
  case UNARY_OP_TYPE_MINUS: return IR_NEG;
  case UNARY_OP_BITWISE_TYPE_COMPLEMENT: return IR_COMPLEMENT;
  }
  MCC_UNREACHABLE();
}

typedef struct IRGenContext {
  Arena* permanent_arena;
  size_t instruction_count;
  IRInstruction* instructions;
  int fresh_variable_counter;
} IRGenContext;

static void push_instruction(IRGenContext* context, IRInstruction instruction)
{
  // TODO: implement vector
  MCC_ASSERT_MSG(context->instruction_count < 16, "Too many instructions!");
  context->instructions[context->instruction_count++] = instruction;
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
  case EXPR_TYPE_CONST:
    return (IRValue){.typ = IR_VALUE_TYPE_CONSTANT,
                     .constant = expr->const_expr.val};
  case EXPR_TYPE_UNARY: {
    const IRValue src =
        emit_ir_instructions_from_expr(expr->unary_op.inner_expr, context);

    const StringView dst_name = create_fresh_variable_name(context);
    const IRValue dst = ir_variable(dst_name);

    const IRInstructionType instruction_type =
        instruction_typ_from_unary_op(expr->unary_op.unary_op_type);

    push_instruction(context, ir_unary_instr(instruction_type, dst, src));

    return dst;
  }
  case EXPR_TYPE_BINARY: MCC_UNIMPLEMENTED(); break;
  }

  MCC_UNREACHABLE();
}

__attribute__((nonnull)) static IRFunctionDef
generate_ir_function_def(const FunctionDecl* decl, Arena* permanent_arena,
                         Arena scratch_arena)

{
  (void)scratch_arena;

  IRGenContext context = (IRGenContext){
      .permanent_arena = permanent_arena,
      .instruction_count = 0,
      .instructions = ARENA_ALLOC_ARRAY(permanent_arena, IRInstruction, 16),
      .fresh_variable_counter = 0};

  for (size_t i = 0; i < decl->body->statement_count; ++i) {
    const Stmt stmt = decl->body->statements[i];
    switch (stmt.type) {
    case STMT_TYPE_RETURN: {
      IRValue return_value =
          emit_ir_instructions_from_expr(stmt.ret.expr, &context);

      push_instruction(&context,
                       ir_single_operand_instr(IR_RETURN, return_value));
      break;
    }
    case STMT_TYPE_COMPOUND: MCC_UNIMPLEMENTED(); break;
    }
    // TODO: handle other kinds of statements
  }

  return (IRFunctionDef){.name = decl->name,
                         .instruction_count = context.instruction_count,
                         .instructions = context.instructions};
}

IRProgram* generate_ir(TranslationUnit* ast, Arena* permanent_arena,
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
