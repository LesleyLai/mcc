#include "x86_helpers.h"

const size_t MAX_INSTRUCTION_COUNT = 160;

X86InstructionVector new_instruction_vector(Arena* arena)
{
  return (X86InstructionVector){
      .length = 0,
      .data = ARENA_ALLOC_ARRAY(arena, X86Instruction, MAX_INSTRUCTION_COUNT)};
}

void push_instruction(X86InstructionVector* instructions,
                      X86Instruction instruction)
{
  MCC_ASSERT_MSG(instructions->length < MAX_INSTRUCTION_COUNT,
                 "Too many instructions");
  instructions->data[instructions->length++] = instruction;
}
