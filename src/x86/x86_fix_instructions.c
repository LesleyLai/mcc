#include "x86_passes.h"

static void fix_binary_instruction(X86InstructionVector* new_instructions,
                                   X86Instruction instruction)
{
  // Fix this binary instruction if both source and destination are memory
  // addresses
  if (instruction.binary.src.typ == X86_OPERAND_STACK &&
      instruction.binary.dest.typ == X86_OPERAND_STACK) {
    const X86Operand temp_register = register_operand(X86_REG_R10);
    push_instruction(
        new_instructions,
        mov(instruction.binary.size, temp_register, instruction.binary.src));

    push_instruction(
        new_instructions,
        binary_instruction(instruction.typ, instruction.binary.size,
                           instruction.binary.dest, temp_register));
  } else {
    push_instruction(new_instructions, instruction);
  }
}

static void fix_shift_instruction(X86InstructionVector* new_instructions,
                                  X86Instruction instruction)
{
  if (instruction.binary.dest.typ == X86_OPERAND_STACK) {
    push_instruction(
        new_instructions,
        mov(X86_SZ_1, register_operand(X86_REG_CX), instruction.binary.src));

    instruction.binary.src = register_operand(X86_REG_CX);
    push_instruction(new_instructions, instruction);
  } else {
    push_instruction(new_instructions, instruction);
  }
}

static void fix_imul_instruction(X86InstructionVector* new_instructions,
                                 X86Instruction instruction)
{
  // imul can't use a memory address as its destination, regardless of its
  // source operand
  if (instruction.binary.dest.typ == X86_OPERAND_STACK) {
    const X86Operand temp_register = register_operand(X86_REG_R11);
    push_instruction(
        new_instructions,
        mov(instruction.binary.size, temp_register, instruction.binary.dest));

    push_instruction(new_instructions,
                     binary_instruction(X86_INST_IMUL, instruction.binary.size,
                                        temp_register, instruction.binary.src));

    push_instruction(
        new_instructions,
        mov(instruction.binary.size, instruction.binary.dest, temp_register));
  } else {
    push_instruction(new_instructions, instruction);
  }
}

static void fix_idiv_instruction(X86InstructionVector* new_instructions,
                                 X86Instruction instruction)
{
  // idiv can't take an immediate operand
  if (instruction.unary.op.typ == X86_OPERAND_IMMEDIATE) {
    const X86Operand temp_register = register_operand(X86_REG_R10);
    push_instruction(
        new_instructions,
        mov(instruction.unary.size, temp_register, instruction.unary.op));
    push_instruction(new_instructions,
                     unary_instruction(X86_INST_IDIV, instruction.unary.size,
                                       temp_register));
  } else {
    push_instruction(new_instructions, instruction);
  }
}

static void fix_cmp_instruction(X86InstructionVector* new_instructions,
                                X86Instruction instruction)
{
  const X86Operand first = instruction.binary.dest;
  const X86Operand second = instruction.binary.src;

  // the first argument of cmp can't be an immediate operands
  const bool first_is_immediate = first.typ == X86_OPERAND_IMMEDIATE;
  const bool both_are_address =
      first.typ == X86_OPERAND_STACK && second.typ == X86_OPERAND_STACK;

  if (first_is_immediate || both_are_address) {
    const X86Operand temp_register = register_operand(X86_REG_R10);
    const X86Size size = instruction.binary.size;

    push_instruction(new_instructions, mov(size, temp_register, first));
    push_instruction(new_instructions, cmp(size, temp_register, second));
  } else {
    push_instruction(new_instructions, instruction);
  }
}

void fix_invalid_instructions(X86FunctionDef* function, uint32_t stack_size,
                              X86CodegenContext* context)
{
  X86InstructionVector new_instructions = {.arena = context->permanent_arena};

  if (stack_size > 0) {
    push_instruction(&new_instructions, allocate_stack(stack_size));
  }
  for (size_t j = 0; j < function->instruction_count; ++j) {
    X86Instruction instruction = function->instructions[j];

    switch (instruction.typ) {
    case X86_INST_MOV:
    case X86_INST_ADD:
    case X86_INST_SUB:
    case X86_INST_AND:
    case X86_INST_OR:
    case X86_INST_XOR:
      fix_binary_instruction(&new_instructions, instruction);
      break;

    case X86_INST_IMUL:
      fix_imul_instruction(&new_instructions, instruction);
      break;

    case X86_INST_IDIV:
      fix_idiv_instruction(&new_instructions, instruction);
      break;
    case X86_INST_SHL:
    case X86_INST_SAR:
      fix_shift_instruction(&new_instructions, instruction);
      break;
    case X86_INST_CMP:
      fix_cmp_instruction(&new_instructions, instruction);
      break;
    default: push_instruction(&new_instructions, instruction);
    }
  }

  function->instruction_count = new_instructions.length;
  function->instructions = new_instructions.data;
}
