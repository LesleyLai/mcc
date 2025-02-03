#include "x86_passes.h"
#include "x86_symbols.h"

#include <mcc/dynarray.h>
#include <mcc/format.h>
#include <mcc/ir.h>
#include <mcc/prelude.h>

static const X86Register arg_registers[] = {X86_REG_DI, X86_REG_SI, //
                                            X86_REG_DX, X86_REG_CX, //
                                            X86_REG_R8, X86_REG_R9};

static X86Operand x86_operand_from_ir(IRValue ir_operand)
{
  switch (ir_operand.typ) {
  case IR_VALUE_TYPE_CONSTANT: return immediate_operand(ir_operand.constant);
  case IR_VALUE_TYPE_VARIABLE: return pseudo_operand(ir_operand.variable);
  }
  MCC_UNREACHABLE();
}

// Unary instructions like neg or not
static void push_unary_instruction(X86InstructionVector* instructions,
                                   X86InstructionType type,
                                   const IRInstruction* ir_instruction)
{
  const X86Operand dst = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand src = x86_operand_from_ir(ir_instruction->operand2);

  push_instruction(instructions,
                   binary_instruction(X86_INST_MOV, X86_SZ_4, dst, src));
  push_instruction(instructions, unary_instruction(type, X86_SZ_4, dst));
}

static void push_binary_instruction(X86InstructionVector* instructions,
                                    X86InstructionType type,
                                    const IRInstruction* ir_instruction)
{
  const X86Operand dst = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand lhs = x86_operand_from_ir(ir_instruction->operand2);
  const X86Operand rhs = x86_operand_from_ir(ir_instruction->operand3);

  push_instruction(instructions,
                   binary_instruction(X86_INST_MOV, X86_SZ_4, dst, lhs));
  push_instruction(instructions, binary_instruction(type, X86_SZ_4, dst, rhs));
}

static void push_div_mod_instruction(X86InstructionVector* instructions,
                                     const IRInstruction* ir_instruction)
{
  const X86Operand dst = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand lhs = x86_operand_from_ir(ir_instruction->operand2);
  const X86Operand rhs = x86_operand_from_ir(ir_instruction->operand3);

  // mov eax, <lhs>
  push_instruction(instructions,
                   binary_instruction(X86_INST_MOV, X86_SZ_4,
                                      register_operand(X86_REG_AX), lhs));

  // cdq
  push_instruction(instructions, (X86Instruction){.typ = X86_INST_CDQ});

  // idiv <rhs>
  push_instruction(instructions,
                   unary_instruction(X86_INST_IDIV, X86_SZ_4, rhs));

  // if div: mov <dst>, eax
  // if mod: mov <dst>, edx
  const X86Operand src =
      register_operand(ir_instruction->typ == IR_DIV ? X86_REG_AX : X86_REG_DX);
  push_instruction(instructions,
                   binary_instruction(X86_INST_MOV, X86_SZ_4, dst, src));
}

static void generate_comparison_instruction(X86InstructionVector* instructions,
                                            const IRInstruction* ir_instruction)
{
  const X86Operand dest = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand lhs = x86_operand_from_ir(ir_instruction->operand2);
  const X86Operand rhs = x86_operand_from_ir(ir_instruction->operand3);

  X86CondCode cond_code;
  switch (ir_instruction->typ) {
  case IR_EQUAL: cond_code = X86_COND_E; break;
  case IR_NOT_EQUAL: cond_code = X86_COND_NE; break;
  case IR_LESS: cond_code = X86_COND_L; break;
  case IR_LESS_EQUAL: cond_code = X86_COND_LE; break;
  case IR_GREATER: cond_code = X86_COND_G; break;
  case IR_GREATER_EQUAL: cond_code = X86_COND_GE; break;
  default: MCC_UNREACHABLE();
  }

  // mov dest, 0
  push_instruction(
      instructions,
      binary_instruction(X86_INST_MOV, X86_SZ_4, dest, immediate_operand(0)));

  // cmp lhs, rhs
  push_instruction(instructions,
                   binary_instruction(X86_INST_CMP, X86_SZ_4, lhs, rhs));

  // setcc dest
  push_instruction(instructions,
                   (X86Instruction){.typ = X86_INST_SETCC,
                                    .setcc = {.cond = cond_code, .op = dest}});
}

static const IRInstruction* get_instruction(const IRFunctionDef* function,
                                            size_t i)
{
  return i < function->instruction_count ? &function->instructions[i] : nullptr;
}

static X86Instruction jmpcc(X86CondCode cond_code, StringView label)
{
  return (X86Instruction){.typ = X86_INST_JMPCC,
                          .jmpcc = {
                              .cond = cond_code,
                              .label = label,
                          }};
}

struct SplitResult {
  uint32_t register_count;
  uint32_t stack_count;
};

// Count the number of parameters/arguments pass via register and via stack
static struct SplitResult count_register_stack_vars(uint32_t count)
{
  const uint32_t register_count = count > 6 ? 6 : count;
  const uint32_t stack_count = count - register_count;
  return (struct SplitResult){
      .register_count = register_count,
      .stack_count = stack_count,
  };
}

static void generate_call_instruction(X86InstructionVector* instructions,
                                      const IRInstruction* ir_instruction,
                                      X86CodegenContext* context)
{
  struct SplitResult arg_counts =
      count_register_stack_vars(ir_instruction->call.arg_count);
  const uint32_t register_arg_count = arg_counts.register_count,
                 stack_arg_count = arg_counts.stack_count;

  // TODO: handle more arguments
  MCC_ASSERT(stack_arg_count == 0);

  for (uint32_t i = 0; i < register_arg_count; ++i) {
    const X86Operand arg = x86_operand_from_ir(ir_instruction->call.args[i]);
    push_instruction(instructions,
                     binary_instruction(X86_INST_MOV, X86_SZ_4,
                                        register_operand(arg_registers[i]),
                                        arg));
  }

  StringView function_name = ir_instruction->call.func_name;
  // On Linux, external functions need to be postfixed with `@PLT`
  if (!has_symbol(context->symbols, function_name)) {
    function_name =
        allocate_printf(context->permanent_arena, "%.*s@PLT",
                        (int)function_name.size, function_name.start);
  }

  push_instruction(instructions, (X86Instruction){
                                     .typ = X86_INST_CALL,
                                     .label = function_name,
                                 });

  const X86Operand dest = x86_operand_from_ir(ir_instruction->call.dest);
  push_instruction(instructions,
                   binary_instruction(X86_INST_MOV, X86_SZ_4, dest,
                                      register_operand(X86_REG_AX)));
}

// First pass to generate assembly. Still need fixing later
X86FunctionDef x86_function_from_ir(const IRFunctionDef* ir_function,
                                    X86CodegenContext* context)
{
  // We use scratch arena here because the instructions generated here will be
  // rewritten
  X86InstructionVector instructions = {.arena = &context->scratch_arena};

  add_symbol(context->symbols, ir_function->name, context->permanent_arena);

  struct SplitResult param_counts =
      count_register_stack_vars(ir_function->param_count);
  const uint32_t register_param_count = param_counts.register_count,
                 stack_param_count = param_counts.stack_count;

  // TODO: handle more parameters
  MCC_ASSERT(stack_param_count == 0);
  for (uint32_t i = 0; i < register_param_count; ++i) {
    // move <parameter>, <arg register>
    push_instruction(&instructions,
                     binary_instruction(X86_INST_MOV, X86_SZ_4,
                                        pseudo_operand(ir_function->params[i]),
                                        register_operand(arg_registers[i])));
  }

  for (size_t i = 0; i < ir_function->instruction_count; ++i) {
    const IRInstruction* ir_instruction = get_instruction(ir_function, i);
    switch (ir_instruction->typ) {
    case IR_INVALID: MCC_UNREACHABLE(); break;
    case IR_RETURN: {
      // move eax, <op>
      push_instruction(&instructions,
                       binary_instruction(
                           X86_INST_MOV, X86_SZ_4, register_operand(X86_REG_AX),
                           x86_operand_from_ir(ir_instruction->operand1)));

      push_instruction(&instructions, (X86Instruction){.typ = X86_INST_RET});
      break;
    }
    case IR_COPY: {
      X86Operand dest = x86_operand_from_ir(ir_instruction->operand1);
      X86Operand src = x86_operand_from_ir(ir_instruction->operand2);
      // mov dest src
      push_instruction(&instructions,
                       binary_instruction(X86_INST_MOV, X86_SZ_4, dest, src));
    } break;
    case IR_NEG:
      push_unary_instruction(&instructions, X86_INST_NEG, ir_instruction);
      break;
    case IR_COMPLEMENT:
      push_unary_instruction(&instructions, X86_INST_NOT, ir_instruction);
      break;
    case IR_NOT: {
      X86Operand dest = x86_operand_from_ir(ir_instruction->operand1);
      X86Operand src = x86_operand_from_ir(ir_instruction->operand2);
      X86Operand zero = immediate_operand(0);
      // mov dest 0
      push_instruction(&instructions,
                       binary_instruction(X86_INST_MOV, X86_SZ_4, dest, zero));

      // cmp src, 0
      push_instruction(&instructions,
                       binary_instruction(X86_INST_CMP, X86_SZ_4, src, zero));

      // sete dest
      push_instruction(
          &instructions,
          (X86Instruction){.typ = X86_INST_SETCC,
                           .setcc = {.cond = X86_COND_E, .op = dest}});
    } break;

    case IR_ADD:
      push_binary_instruction(&instructions, X86_INST_ADD, ir_instruction);
      break;
    case IR_SUB:
      push_binary_instruction(&instructions, X86_INST_SUB, ir_instruction);
      break;
    case IR_MUL:
      push_binary_instruction(&instructions, X86_INST_IMUL, ir_instruction);
      break;
    case IR_DIV:
    case IR_MOD: push_div_mod_instruction(&instructions, ir_instruction); break;
    case IR_BITWISE_AND:
      push_binary_instruction(&instructions, X86_INST_AND, ir_instruction);
      break;
    case IR_BITWISE_OR:
      push_binary_instruction(&instructions, X86_INST_OR, ir_instruction);
      break;
    case IR_BITWISE_XOR:
      push_binary_instruction(&instructions, X86_INST_XOR, ir_instruction);
      break;
    case IR_SHIFT_LEFT:
      push_binary_instruction(&instructions, X86_INST_SHL, ir_instruction);
      break;
    case IR_SHIFT_RIGHT_ARITHMETIC:
      push_binary_instruction(&instructions, X86_INST_SAR, ir_instruction);
      break;
    case IR_SHIFT_RIGHT_LOGICAL: MCC_UNIMPLEMENTED(); break;
    case IR_EQUAL:
    case IR_NOT_EQUAL:
    case IR_LESS:
    case IR_LESS_EQUAL:
    case IR_GREATER:
    case IR_GREATER_EQUAL:
      generate_comparison_instruction(&instructions, ir_instruction);
      break;
    case IR_JMP:
      push_instruction(&instructions, (X86Instruction){
                                          .typ = X86_INST_JMP,
                                          .label = ir_instruction->label,
                                      });
      break;
    case IR_BR: {
      const X86Operand cond = x86_operand_from_ir(ir_instruction->cond);
      const StringView if_label = ir_instruction->if_label;
      const StringView else_label = ir_instruction->else_label;

      // cmp cond, 0
      push_instruction(&instructions,
                       binary_instruction(X86_INST_CMP, X86_SZ_4, cond,
                                          immediate_operand(0)));

      const IRInstruction* next_instruction =
          get_instruction(ir_function, i + 1);

      bool skip_if = false;
      bool skip_else = false;

      if (next_instruction != nullptr && next_instruction->typ == IR_LABEL) {
        if (str_eq(next_instruction->label, if_label)) {
          // If the if_label is directly follow this instruction
          skip_if = true;
        } else if (str_eq(next_instruction->label, else_label)) {
          // If the else_label is directly follow this instruction
          skip_else = true;
        }
      }

      MCC_ASSERT_MSG(
          !(skip_if && skip_else),
          "Need to at least generate jump instruction for one branch");
      if (!skip_if) {
        // jne .if_label
        push_instruction(&instructions, jmpcc(X86_COND_NE, if_label));
      }
      if (!skip_else) {
        // je .else_label
        push_instruction(&instructions, jmpcc(X86_COND_E, else_label));
      }

    } break;
    case IR_LABEL:
      push_instruction(&instructions, (X86Instruction){
                                          .typ = X86_INST_LABEL,
                                          .label = ir_instruction->label,
                                      });
      break;
    case IR_CALL: {
      generate_call_instruction(&instructions, ir_instruction, context);
      break;
    }
    }
  }

  return (X86FunctionDef){.name = ir_function->name,
                          .instructions = instructions.data,
                          .instruction_count = instructions.length};
}
