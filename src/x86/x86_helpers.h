#ifndef MCC_X86_HELPERS_H
#define MCC_X86_HELPERS_H

#include <mcc/x86.h>

#include <stdint.h>
#include <stdlib.h>

// A dynamically-sized array of instructions
struct X86InstructionVector {
  uint32_t length;
  uint32_t capacity;
  X86Instruction* data;
  Arena* arena;
};

typedef struct X86InstructionVector X86InstructionVector;
void push_instruction(X86InstructionVector* instructions,
                      X86Instruction instruction);

#define X86_UNARY_INSTRUCTION_CASES                                            \
  case X86_INST_NEG:                                                           \
  case X86_INST_NOT:                                                           \
  case X86_INST_IDIV

#define X86_BINARY_INSTRUCTION_CASES                                           \
  case X86_INST_MOV:                                                           \
  case X86_INST_ADD:                                                           \
  case X86_INST_SUB:                                                           \
  case X86_INST_IMUL:                                                          \
  case X86_INST_AND:                                                           \
  case X86_INST_OR:                                                            \
  case X86_INST_XOR:                                                           \
  case X86_INST_SHL:                                                           \
  case X86_INST_SAR:                                                           \
  case X86_INST_CMP

// =============================================================================
// Convenient "constructors"
// =============================================================================
static inline X86Operand immediate_operand(int32_t value)
{
  return (X86Operand){.typ = X86_OPERAND_IMMEDIATE, .imm = value};
}

static inline X86Operand register_operand(X86Register reg)
{
  return (X86Operand){.typ = X86_OPERAND_REGISTER, .reg = reg};
}

static inline X86Operand stack_operand(size_t offset)
{
  return (X86Operand){.typ = X86_OPERAND_STACK, .stack = {.offset = offset}};
}

// Constructs an x86 unary instruction
X86Instruction unary_instruction(X86InstructionType type, X86Size size,
                                 X86Operand op);

// Constructs an x86 binary instruction
X86Instruction binary_instruction(X86InstructionType type, X86Size size,
                                  X86Operand dest, X86Operand src);

#endif // MCC_X86_HELPERS_H
