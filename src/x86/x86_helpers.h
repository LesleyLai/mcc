#ifndef MCC_X86_HELPERS_H
#define MCC_X86_HELPERS_H

#include <mcc/x86.h>

#include <stdint.h>
#include <stdlib.h>

// instruction vector
// TODO: implement dynamic resizing
struct X86InstructionVector {
  size_t length;
  X86Instruction* data;
};

typedef struct X86InstructionVector X86InstructionVector;
X86InstructionVector new_instruction_vector(Arena* arena);
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

// helpers
static inline X86Operand x86_immediate_operand(int32_t value)
{
  return (X86Operand){.typ = X86_OPERAND_IMMEDIATE, .imm = value};
}

static inline X86Operand x86_register_operand(X86Register reg)
{
  return (X86Operand){.typ = X86_OPERAND_REGISTER, .reg = reg};
}

static inline X86Operand x86_stack_operand(size_t offset)
{
  return (X86Operand){.typ = X86_OPERAND_STACK, .stack = {.offset = offset}};
}

static inline X86Instruction x86_unary_instruction(X86InstructionType type,
                                                   X86Size size, X86Operand op)
{
  // TODO: verify type
  return (X86Instruction){.typ = type,
                          .unary = {
                              .size = size,
                              .op = op,
                          }};
}

static inline bool is_binary(X86InstructionType typ)
{
  switch (typ) {
  case x86_INST_INVALID:
    MCC_UNREACHABLE();
  X86_BINARY_INSTRUCTION_CASES:
    return true;
  case X86_INST_NOP:
  case X86_INST_RET:
  case X86_INST_CDQ:
  X86_UNARY_INSTRUCTION_CASES:
  case X86_INST_SETCC: return false;
  }
  MCC_UNREACHABLE();
}

static inline X86Instruction x86_binary_instruction(X86InstructionType type,
                                                    X86Size size,
                                                    X86Operand dest,
                                                    X86Operand src)
{
  MCC_ASSERT_MSG(is_binary(type),
                 "Can't construct binary instruction from non-binary type");
  return (X86Instruction){.typ = type,
                          .binary = {
                              .size = size,
                              .dest = dest,
                              .src = src,
                          }};
}

#endif // MCC_X86_HELPERS_H
