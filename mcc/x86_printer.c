#include "x86.h"

#include "utils/prelude.h"

static const char* x86_register_name(X86Register reg)
{
  switch (reg) {
  case X86_REG_INVALID: MCC_UNREACHABLE(); break;
  case X86_REG_AX: return "eax";
  case X86_REG_R10: return "r10d";
  case X86_REG_R11: return "r11d";
  case X86_REG_SP: return "rsp";
  }
  MCC_UNREACHABLE();
}

static void print_x86_operand(X86Operand operand, FILE* stream)
{
  switch (operand.typ) {
  case X86_OPERAND_INVALID: MCC_UNREACHABLE(); break;
  case X86_OPERAND_IMMEDIATE: (void)fprintf(stream, "%i", operand.imm); break;
  case X86_OPERAND_REGISTER:
    (void)fprintf(stream, "%s", x86_register_name(operand.reg));
    break;
  case X86_OPERAND_PSEUDO:
    (void)fprintf(stream, "%*s", (int)operand.pseudo.size,
                  operand.pseudo.start);
    break;
  case X86_OPERAND_STACK:
    (void)fprintf(stream, "dword ptr [rbp-%li]", operand.stack.offset);
    break;
  }
}

static void print_binary_instruction(const char* name,
                                     X86Instruction instruction, FILE* stream)
{
  (void)fprintf(stream, "  %-6s ", name);
  print_x86_operand(instruction.operand1, stream);
  (void)fputs(", ", stream);
  print_x86_operand(instruction.operand2, stream);
}

void x86_print_instruction(X86Instruction instruction, FILE* stream)
{
  switch (instruction.typ) {
  case x86_INST_INVALID: MCC_UNREACHABLE(); break;
  case X86_INST_NOP: MCC_UNIMPLEMENTED(); break;
  case X86_INST_MOV:
    print_binary_instruction("mov", instruction, stream);
    break;
  case X86_INST_RET: {
    (void)fputs("  mov    rsp, rbp\n", stream);
    (void)fputs("  pop    rbp\n", stream);
    (void)fputs("  ret\n", stream);
  } break;
  case X86_INST_NEG:
    (void)fputs("  neg    ", stream);
    print_x86_operand(instruction.operand1, stream);
    break;
  case X86_INST_NOT:
    (void)fputs("  not    ", stream);
    print_x86_operand(instruction.operand1, stream);
    break;
  case X86_INST_ADD:
    print_binary_instruction("add", instruction, stream);
    break;
  case X86_INST_SUB:
    print_binary_instruction("sub", instruction, stream);
    break;
  case X86_INST_IMUL:
    print_binary_instruction("imul", instruction, stream);
    break;
  }
}

static void dump_x86_function(const X86FunctionDef* function, FILE* stream)
{
  (void)fprintf(stream, "%.*s:\n", (int)function->name.size,
                function->name.start);
  // function prolog
  (void)fputs("  push   rbp\n", stream);
  (void)fputs("  mov    rbp, rsp\n", stream);

  for (size_t i = 0; i < function->instruction_count; ++i) {
    x86_print_instruction(function->instructions[i], stream);
    (void)fprintf(stream, "\n");
  }
}

void x86_dump_assembly(const X86Program* program, FILE* stream)
{
  (void)fputs(".intel_syntax noprefix\n", stream);

  for (size_t i = 0; i < program->function_count; ++i) {
    (void)fprintf(stream, ".globl %.*s\n", (int)program->functions->name.size,
                  program->functions->name.start);
    dump_x86_function(&program->functions[i], stream);
  }

#ifdef __linux__
  // indicates that code does not need an execution stack
  (void)fprintf(stream, ".section .note.GNU-stack,\"\",@progbits\n");
#endif
}
