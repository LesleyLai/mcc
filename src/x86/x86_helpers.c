#include "x86_helpers.h"
#include <mcc/dynarray.h>

void push_instruction(X86InstructionVector* instructions,
                      X86Instruction instruction)
{
  DYNARRAY_PUSH_BACK(instructions, X86Instruction, instructions->arena,
                     instruction);
}

X86Instruction unary_instruction(X86InstructionType type, X86Size size,
                                 X86Operand op)
{
  // TODO: verify type
  return (X86Instruction){.typ = type,
                          .unary = {
                              .size = size,
                              .op = op,
                          }};
}

static bool is_binary(X86InstructionType typ)
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

X86Instruction binary_instruction(X86InstructionType type, X86Size size,
                                  X86Operand dest, X86Operand src)
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
