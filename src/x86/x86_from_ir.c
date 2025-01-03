#include "x86_passes.h"
#include <mcc/ir.h>

static X86Operand x86_operand_from_ir(IRValue ir_operand)
{
  switch (ir_operand.typ) {
  case IR_VALUE_TYPE_CONSTANT: return immediate_operand(ir_operand.constant);
  case IR_VALUE_TYPE_VARIABLE:
    return (X86Operand){.typ = X86_OPERAND_PSEUDO,
                        .pseudo = ir_operand.variable};
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

// First pass to generate assembly. Still need fixing later
X86FunctionDef x86_function_from_ir(const IRFunctionDef* ir_function,
                                         Arena* scratch_arena)
{
  X86InstructionVector instructions = {.arena = scratch_arena};

  for (size_t j = 0; j < ir_function->instruction_count; ++j) {
    const IRInstruction* ir_instruction = &ir_function->instructions[j];
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
    }
  }

  return (X86FunctionDef){.name = ir_function->name,
                          .instructions = instructions.data,
                          .instruction_count = instructions.length};
}
