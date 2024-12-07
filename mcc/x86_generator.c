#include "ir.h"
#include "x86.h"

static X86Operand x86_immediate_operand(int32_t value)
{
  return (X86Operand){.typ = X86_OPERAND_IMMEDIATE, .imm = value};
}

X86Program generate_x86_assembly(IRProgram* ir, Arena* permanent_arena)
{
  const size_t function_count = ir->function_count;
  X86FunctionDef* functions =
      ARENA_ALLOC_ARRAY(permanent_arena, X86FunctionDef, function_count);

  for (size_t i = 0; i < ir->function_count; ++i) {
    IRFunctionDef* ir_function = &ir->functions[i];
    X86FunctionDef* x86_function = &functions[i];

    // TODO: implement vector
    const size_t MAX_INSTRUCTION_COUNT = 16;
    X86Instruction* instructions = ARENA_ALLOC_ARRAY(
        permanent_arena, X86Instruction, MAX_INSTRUCTION_COUNT);

    size_t instruction_count = 0;
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

          instructions[instruction_count++] = (X86Instruction){
              .typ = X86_INST_MOV,
              .operand1 = (X86Operand){.typ = X86_OPERAND_REGISTER},
              .operand2 = x86_immediate_operand(value),
          };

          // TODO: add mov

          instructions[instruction_count++] =
              (X86Instruction){.typ = X86_INST_RET};

          break;
        }
        case IR_VALUE_TYPE_VARIABLE: MCC_UNIMPLEMENTED(); break;
        }
      }
      }
    }

    (*x86_function) = (X86FunctionDef){.name = ir->functions[i].name,
                                       .instructions = instructions,
                                       .instruction_count = instruction_count};
  }

  return (X86Program){.function_count = function_count, .functions = functions};
}
