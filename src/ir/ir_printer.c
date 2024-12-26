#include <mcc/ir.h>

static void print_ir_value(IRValue value)
{
  switch (value.typ) {
  case IR_VALUE_TYPE_CONSTANT: printf("%i", value.constant); break;
  case IR_VALUE_TYPE_VARIABLE:
    printf("%.*s", (int)value.variable.size, value.variable.start);
    break;
  }
}

static void print_unary_op(IRInstruction instruction, const char* op_name)
{
  printf("  ");
  print_ir_value(instruction.operand1); // dest
  printf(" = %s ", op_name);
  print_ir_value(instruction.operand2); // src
  printf("\n");
}

static void print_binary_op(IRInstruction instruction, const char* op_name)
{
  printf("  ");
  print_ir_value(instruction.operand1); // dest
  printf(" = %s ", op_name);
  print_ir_value(instruction.operand2); // lhs
  printf(" ");
  print_ir_value(instruction.operand3); // rhs
  printf("\n");
}

void print_ir(const IRProgram* ir)
{
  for (size_t i = 0; i < ir->function_count; i++) {
    const IRFunctionDef ir_function = ir->functions[i];
    const StringView name = ir_function.name;
    printf("func %.*s():\n", (int)name.size, name.start);
    for (size_t j = 0; j < ir_function.instruction_count; j++) {
      const IRInstruction instruction = ir_function.instructions[j];
      switch (instruction.typ) {
      case IR_INVALID: MCC_UNREACHABLE(); break;
      case IR_RETURN: {
        printf("  return ");
        print_ir_value(instruction.operand1);
        printf("\n");
      } break;
      case IR_NEG: print_unary_op(instruction, "neg"); break;
      case IR_COMPLEMENT: print_unary_op(instruction, "complement"); break;
      case IR_ADD: print_binary_op(instruction, "add"); break;
      case IR_SUB: print_binary_op(instruction, "sub"); break;
      case IR_MUL: print_binary_op(instruction, "mul"); break;
      case IR_DIV: print_binary_op(instruction, "div"); break;
      case IR_MOD: print_binary_op(instruction, "mod"); break;
      }
    }
  }
}