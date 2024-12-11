#include "ir.h"
#include "x86.h"

const size_t MAX_INSTRUCTION_COUNT = 16;

// TODO: implement dynamic resizing
typedef struct X86InstructionVector {
  size_t length;
  X86Instruction* data;
} X86InstructionVector;

static X86InstructionVector new_instruction_vector(Arena* permanent_arena)
{
  return (X86InstructionVector){
      .length = 0,
      .data = ARENA_ALLOC_ARRAY(permanent_arena, X86Instruction,
                                MAX_INSTRUCTION_COUNT)};
}

static void push_instruction(struct X86InstructionVector* instructions,
                             X86Instruction instruction)
{
  MCC_ASSERT_MSG(instructions->length < MAX_INSTRUCTION_COUNT + 1,
                 "Too many instructions");
  instructions->data[instructions->length++] = instruction;
}

static X86Operand x86_immediate_operand(int32_t value)
{
  return (X86Operand){.typ = X86_OPERAND_IMMEDIATE, .imm = value};
}

static X86FunctionDef
generate_x86_function_def(const IRFunctionDef* ir_function,
                          Arena* permanent_arena)
{
  X86InstructionVector instructions = new_instruction_vector(permanent_arena);

  for (size_t j = 0; j < ir_function->instruction_count; ++j) {
    IRInstruction* ir_instruction = &ir_function->instructions[j];
    switch (ir_instruction->typ) {
    case IR_NEG: MCC_UNIMPLEMENTED(); break;
    case IR_COMPLEMENT: MCC_UNIMPLEMENTED(); break;
    case IR_RETURN: {
      const IRValue operand = ir_instruction->value1;
      switch (operand.typ) {
      case IR_VALUE_TYPE_INVALID: MCC_UNREACHABLE(); break;
      case IR_VALUE_TYPE_CONSTANT: {
        const int value = operand.constant;
        push_instruction(
            &instructions,
            (X86Instruction){
                .typ = X86_INST_MOV,
                .operand1 = (X86Operand){.typ = X86_OPERAND_REGISTER},
                .operand2 = x86_immediate_operand(value),
            });

        push_instruction(&instructions, (X86Instruction){.typ = X86_INST_RET});
        break;
      }
      case IR_VALUE_TYPE_VARIABLE: MCC_UNIMPLEMENTED(); break;
      }
    }
    }
  }

  return (X86FunctionDef){.name = ir_function->name,
                          .instructions = instructions.data,
                          .instruction_count = instructions.length};
}

X86Program generate_x86_assembly(IRProgram* ir, Arena* permanent_arena)
{
  const size_t function_count = ir->function_count;
  X86FunctionDef* functions =
      ARENA_ALLOC_ARRAY(permanent_arena, X86FunctionDef, function_count);

  for (size_t i = 0; i < ir->function_count; ++i) {
    functions[i] =
        generate_x86_function_def(&ir->functions[i], permanent_arena);
  }

  return (X86Program){.function_count = function_count, .functions = functions};
}
