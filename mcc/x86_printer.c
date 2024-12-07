#include "x86.h"

#include "utils/prelude.h"

static const char* x86_instruction_name(X86InstructionType typ)
{
  switch (typ) {
  case X86_INST_NOP: return "nop";
  case X86_INST_RET: return "ret";
  case X86_INST_MOV: return "mov";
  }
  MCC_UNREACHABLE();
}

static void print_x86_operand(X86Operand operand, FILE* stream)
{
  switch (operand.typ) {
  case X86_OPERAND_IMMEDIATE: (void)fprintf(stream, "%i", operand.imm); break;
  case X86_OPERAND_REGISTER: (void)fprintf(stream, "rax"); break;
  }
}

static void dump_x86_function(const X86FunctionDef* function, FILE* stream)
{
  (void)fprintf(stream, "%.*s:\n", (int)function->name.size,
                function->name.start);
  for (size_t i = 0; i < function->instruction_count; ++i) {
    const X86Instruction* instruction = &function->instructions[i];
    (void)fprintf(stream, "  %s", x86_instruction_name(instruction->typ));
    switch (instruction->typ) {
    case X86_INST_MOV:
      (void)fputs(" ", stream);
      print_x86_operand(instruction->operand1, stream);
      (void)fputs(", ", stream);
      print_x86_operand(instruction->operand2, stream);
      break;
    default: break;
    }
    (void)fprintf(stream, "\n");
  }
}

void dump_x86_assembly(const X86Program* program, FILE* stream)
{
  for (size_t i = 0; i < program->function_count; ++i) {
    (void)fprintf(stream, ".globl %.*s\n", (int)program->functions->name.size,
                  program->functions->name.start);
    dump_x86_function(&program->functions[i], stream);
  }
}
