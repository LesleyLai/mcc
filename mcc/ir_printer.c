#include "ir.h"

static void print_ir_value(IRValue value)
{
  switch (value.typ) {
  case IR_VALUE_TYPE_CONSTANT: printf("%i", value.constant); break;
  case IR_VALUE_TYPE_VARIABLE:
    printf("%.*s", (int)value.variable.size, value.variable.start);
    break;
  }
}

static void print_unary_op(IRValue dest, IRValue src, const char* op_name)
{
  printf("  ");
  print_ir_value(dest);
  printf(" = %s ", op_name);
  print_ir_value(src);
  printf("\n");
}

void dump_ir(const IRProgram* ir)
{
  for (size_t i = 0; i < ir->function_count; i++) {
    const IRFunctionDef ir_function = ir->functions[i];
    const StringView name = ir_function.name;
    printf("func %.*s():\n", (int)name.size, name.start);
    for (size_t j = 0; j < ir_function.instruction_count; j++) {
      const IRInstruction instruction = ir_function.instructions[j];
      switch (instruction.typ) {
      case IR_NEG:
        print_unary_op(instruction.operand1, instruction.operand2, "neg");
        break;
      case IR_COMPLEMENT:
        print_unary_op(instruction.operand1, instruction.operand2, "complement");
        break;
      case IR_RETURN: {
        printf("  return ");
        print_ir_value(instruction.operand1);
        printf("\n");
      } break;
      }
    }
  }
}
