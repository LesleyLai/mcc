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
      case IR_COPY: print_unary_op(instruction, "copy"); break;
      case IR_NEG: print_unary_op(instruction, "neg"); break;
      case IR_COMPLEMENT: print_unary_op(instruction, "complement"); break;
      case IR_NOT: print_unary_op(instruction, "not"); break;
      case IR_ADD: print_binary_op(instruction, "add"); break;
      case IR_SUB: print_binary_op(instruction, "sub"); break;
      case IR_MUL: print_binary_op(instruction, "mul"); break;
      case IR_DIV: print_binary_op(instruction, "div"); break;
      case IR_MOD: print_binary_op(instruction, "mod"); break;
      case IR_BITWISE_AND: print_binary_op(instruction, "bitand"); break;
      case IR_BITWISE_OR: print_binary_op(instruction, "bitor"); break;
      case IR_BITWISE_XOR: print_binary_op(instruction, "xor"); break;
      case IR_SHIFT_LEFT: print_binary_op(instruction, "shl"); break;
      case IR_SHIFT_RIGHT_ARITHMETIC:
        print_binary_op(instruction, "ashr");
        break;
      case IR_SHIFT_RIGHT_LOGICAL: print_binary_op(instruction, "lshr"); break;
      case IR_EQUAL: print_binary_op(instruction, "eq"); break;
      case IR_NOT_EQUAL: print_binary_op(instruction, "ne"); break;
      case IR_LESS: print_binary_op(instruction, "lt"); break;
      case IR_LESS_EQUAL: print_binary_op(instruction, "le"); break;
      case IR_GREATER: print_binary_op(instruction, "gt"); break;
      case IR_GREATER_EQUAL: print_binary_op(instruction, "ge"); break;
      case IR_JMP: {
        printf("  jmp ");
        printf(".%.*s\n", (int)instruction.label.size, instruction.label.start);
      } break;
      case IR_BR: {
        printf("  br ");
        print_ir_value(instruction.cond);
        printf(" .%.*s .%.*s\n",                                           //
               (int)instruction.if_label.size, instruction.if_label.start, //
               (int)instruction.else_label.size, instruction.else_label.start);
      } break;
      case IR_LABEL: {
        printf(".%.*s:\n", (int)instruction.label.size,
               instruction.label.start);
      } break;
      case IR_CALL: {
        MCC_UNIMPLEMENTED();
      } break;
      }
    }
  }
}
