#include "gen_ir.h"

#include <stdio.h>

// TODO: use a vector
#define MAX_INSTRUCTIONS_IN_FUNCTION 1024
#define MAX_FUNCTIONS_IN_TU 256

static IR_Instruction ir_ret(Register rdest)
{
  return (IR_Instruction){.opcode = IR_RETURN,
                          .optype = IR_Type_I32,
                          .data.return_i32 = {.rdest = rdest}};
}

static IR_Instruction ir_load_i32(Register rdest, int32_t value)
{
  return (IR_Instruction){
      .opcode = IR_LOAD_LITERAL,
      .optype = IR_Type_I32,
      .data.load_i32_literal = {.rdest = rdest, .value = value}};
}

static IR_Instruction ir_binary_op(enum IR_Opcode opcode, Register r1,
                                   Register r2, Register rdest)
{
  return (IR_Instruction){.opcode = (uint8_t)opcode,
                          .optype = IR_Type_I32,
                          .data.binary_op = {
                              .r1 = r1,
                              .r2 = r2,
                              .rdest = rdest,
                          }};
}

static void emit(IR_Function* output, IR_Instruction instr,
                 PolyAllocator* allocator)
{
  if (output->size == 0) {
    output->instructions = POLY_ALLOC_ARRAY(allocator, IR_Instruction,
                                            MAX_INSTRUCTIONS_IN_FUNCTION);
  }

  // TODO
  if (output->size >= MAX_INSTRUCTIONS_IN_FUNCTION - 1) {
    fprintf(stderr, "Cannot handle more than %d instructions!",
            MAX_INSTRUCTIONS_IN_FUNCTION);
    exit(1);
  }

  output->instructions[output->size++] = instr;
}

static Register new_register(IR_Function* func)
{
  return func->register_count++;
}

static Register generate_binary_expr(IR_Function* output, enum IR_Opcode opcode,
                                     const Expr* lhs, const Expr* rhs,
                                     PolyAllocator* allocator);

static Register generate_expr(IR_Function* output, const Expr* expr,
                              PolyAllocator* allocator)
{
  switch (expr->type) {
  case CONST_EXPR: {
    const Register r = new_register(output);
    emit(output, ir_load_i32(r, expr->const_expr.val), allocator);
    return r;
  }

  case BINARY_OP_EXPR: {
    switch (expr->binary_op.binary_op_type) {
    case BINARY_OP_PLUS:
      return generate_binary_expr(output, IR_ADD, expr->binary_op.lhs,
                                  expr->binary_op.rhs, allocator);
    case BINARY_OP_MINUS:
      return generate_binary_expr(output, IR_SUB, expr->binary_op.lhs,
                                  expr->binary_op.rhs, allocator);
    case BINARY_OP_MULT:
      return generate_binary_expr(output, IR_MUL, expr->binary_op.lhs,
                                  expr->binary_op.rhs, allocator);
    case BINARY_OP_DIVIDE:
      return generate_binary_expr(output, IR_DIV, expr->binary_op.lhs,
                                  expr->binary_op.rhs, allocator);
    }
    // todo
    return 0;
  }
  }

  // unreachable
  return 0;
}

static Register generate_binary_expr(IR_Function* output, enum IR_Opcode opcode,
                                     const Expr* lhs, const Expr* rhs,
                                     PolyAllocator* allocator)
{
  const Register r1 = generate_expr(output, lhs, allocator);
  const Register r2 = generate_expr(output, rhs, allocator);
  const Register rdest = new_register(output);

  emit(output, ir_binary_op(opcode, r1, r2, rdest), allocator);
  return rdest;
}

static void generate_stmt(IR_Function* output, const Stmt* stmt,
                          PolyAllocator* allocator)
{
  switch (stmt->type) {
  case COMPOUND_STMT: {
    for (size_t i = 0; i < stmt->compound.statement_count; ++i) {
      generate_stmt(output, &stmt->compound.statements[i], allocator);
    }
  }
    return;
  case RETURN_STMT: {
    const Register r = generate_expr(output, stmt->ret.expr, allocator);
    emit(output, ir_ret(r), allocator);
  }
    return;
  }
}

IR_Function generate_function(FunctionDecl* func, PolyAllocator* allocator)
{
  IR_Function ir_function = (IR_Function){
      .name = string_buffer_from_view(func->name, allocator),
      .size = 0,
      .instructions = NULL,
      .register_count = 0,
  };

  for (size_t i = 0; i < func->body->statement_count; ++i) {
    generate_stmt(&ir_function, &func->body->statements[i], allocator);
  }

  return ir_function;
}

IR_Module* generate_ir(TranslationUnit* tu, PolyAllocator* allocator)
{
  if (tu->decl_count > MAX_FUNCTIONS_IN_TU) {
    fprintf(stderr, "MCC cannot handle more than %d functions yet",
            MAX_FUNCTIONS_IN_TU);
    exit(1);
  }

  IR_Module* module = POLY_ALLOC_OBJECT(allocator, IR_Module);
  (*module) = (IR_Module){
      .function_count = 0,
      .functions = NULL,
  };

  if (tu->decl_count > 0) {
    module->functions =
        POLY_ALLOC_ARRAY(allocator, IR_Function, MAX_FUNCTIONS_IN_TU);
  }

  for (size_t i = 0; i < tu->decl_count; ++i) {
    module->functions[module->function_count++] =
        generate_function(&tu->decls[i], allocator);
  }

  return module;
}
