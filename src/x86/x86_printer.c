#include <mcc/prelude.h>
#include <mcc/x86.h>

static const char* x86_register_name(X86Register reg, X86Size size)
{
  switch (size) {
  case X86_SZ_1: {
    switch (reg) {
    case X86_REG_INVALID: MCC_UNREACHABLE(); break;
    case X86_REG_AX: return "al";
    case X86_REG_BX: return "bl";
    case X86_REG_CX: return "cl";
    case X86_REG_DX: return "dl";
    case X86_REG_DI: return "dil";
    case X86_REG_SI: return "sil";
    case X86_REG_R8: return "r8b";
    case X86_REG_R9: return "r9b";
    case X86_REG_R10: return "r10b";
    case X86_REG_R11: return "r11b";
    case X86_REG_SP: return "spl";
    }
    MCC_UNREACHABLE();
  }
  case X86_SZ_2: MCC_UNREACHABLE(); break;
  case X86_SZ_4: {
    switch (reg) {
    case X86_REG_INVALID: MCC_UNREACHABLE(); break;
    case X86_REG_AX: return "eax";
    case X86_REG_BX: return "ebx";
    case X86_REG_CX: return "ecx";
    case X86_REG_DX: return "edx";
    case X86_REG_DI: return "edi";
    case X86_REG_SI: return "esi";
    case X86_REG_R8: return "r8d";
    case X86_REG_R9: return "r9d";
    case X86_REG_R10: return "r10d";
    case X86_REG_R11: return "r11d";
    case X86_REG_SP: return "esp";
    }
    MCC_UNREACHABLE();
  }
  case X86_SZ_8: {
    switch (reg) {
    case X86_REG_INVALID: MCC_UNREACHABLE(); break;
    case X86_REG_AX: return "rax";
    case X86_REG_BX: return "rbx";
    case X86_REG_CX: return "rcx";
    case X86_REG_DX: return "rdx";
    case X86_REG_DI: return "rdi";
    case X86_REG_SI: return "rsi";
    case X86_REG_R8: return "r8";
    case X86_REG_R9: return "r9";
    case X86_REG_R10: return "r10";
    case X86_REG_R11: return "r11";
    case X86_REG_SP: return "rsp";
    }
    MCC_UNREACHABLE();
  }
  }
  MCC_UNREACHABLE();
}

static const char* size_directive(X86Size size)
{
  switch (size) {
  case X86_SZ_1: return "byte ptr";
  case X86_SZ_2: return "word ptr";
  case X86_SZ_4: return "dword ptr";
  case X86_SZ_8: return "qword ptr";
  }
  MCC_UNREACHABLE();
}

static void print_x86_operand(X86Operand operand, X86Size size, FILE* stream)
{
  switch (operand.typ) {
  case X86_OPERAND_INVALID: MCC_UNREACHABLE(); break;
  case X86_OPERAND_IMMEDIATE: {
    (void)fprintf(stream, "%i", operand.imm);
  } break;
  case X86_OPERAND_REGISTER:
    (void)fprintf(stream, "%s", x86_register_name(operand.reg, size));
    break;
  case X86_OPERAND_PSEUDO:
    (void)fprintf(stream, "%.*s", (int)operand.pseudo.size,
                  operand.pseudo.start);
    break;
  case X86_OPERAND_STACK:
    if (operand.stack.offset > 0) {
      (void)fprintf(stream, "%s [rbp-%li]", size_directive(size),
                    operand.stack.offset);
    } else if (operand.stack.offset < 0) {
      (void)fprintf(stream, "%s [rbp+%li]", size_directive(size),
                    -operand.stack.offset);
    } else {
      (void)fprintf(stream, "%s rbp", size_directive(size));
    }
    break;
  case X86_OPERAND_DATA:
    (void)fprintf(stream, "%s [rip + %.*s]", size_directive(size),
                  (int)operand.data.size, operand.data.start);
    break;
  }
}

static void print_cond_jmp_instruction(X86Instruction instruction, FILE* stream)
{
  const char* name = nullptr;
  switch (instruction.jmpcc.cond) {
  case X86_COND_INVALID: MCC_UNREACHABLE(); break;
  case X86_COND_E: name = "je"; break;
  case X86_COND_NE: name = "jne"; break;
  case X86_COND_G: name = "jg"; break;
  case X86_COND_GE: name = "jge"; break;
  case X86_COND_L: name = "jl"; break;
  case X86_COND_LE: name = "jle"; break;
  }
  (void)fprintf(stream, "  %-6s ", name);
  (void)fprintf(stream, ".L%.*s", (int)instruction.jmpcc.label.size,
                instruction.jmpcc.label.start);
}

static void print_cond_set_instruction(X86Instruction instruction, FILE* stream)
{
  const char* name = nullptr;
  switch (instruction.setcc.cond) {
  case X86_COND_INVALID: MCC_UNREACHABLE(); break;
  case X86_COND_E: name = "sete"; break;
  case X86_COND_NE: name = "setne"; break;
  case X86_COND_G: name = "setg"; break;
  case X86_COND_GE: name = "setge"; break;
  case X86_COND_L: name = "setl"; break;
  case X86_COND_LE: name = "setle"; break;
  }
  (void)fprintf(stream, "  %-6s ", name);
  print_x86_operand(instruction.setcc.op, X86_SZ_1, stream);
}

static void print_unary_instruction(const char* name,
                                    X86Instruction instruction, FILE* stream)
{
  (void)fprintf(stream, "  %-6s ", name);
  print_x86_operand(instruction.unary.op, instruction.unary.size, stream);
}

static void print_binary_instruction(const char* name,
                                     X86Instruction instruction, FILE* stream)
{
  (void)fprintf(stream, "  %-6s ", name);
  print_x86_operand(instruction.binary.dest, instruction.binary.size, stream);
  (void)fputs(", ", stream);
  print_x86_operand(instruction.binary.src, instruction.binary.size, stream);
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
  case X86_INST_NEG: print_unary_instruction("neg", instruction, stream); break;
  case X86_INST_NOT: print_unary_instruction("not", instruction, stream); break;
  case X86_INST_PUSH:
    print_unary_instruction("push", instruction, stream);
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
  case X86_INST_IDIV:
    print_unary_instruction("idiv", instruction, stream);
    break;
  case X86_INST_CDQ: (void)fputs("  cdq", stream); break;
  case X86_INST_AND:
    print_binary_instruction("and", instruction, stream);
    break;
  case X86_INST_OR: print_binary_instruction("or", instruction, stream); break;
  case X86_INST_XOR:
    print_binary_instruction("xor", instruction, stream);
    break;
  case X86_INST_SHL:
    print_binary_instruction("shl", instruction, stream);
    break;
  case X86_INST_SAR:
    print_binary_instruction("sar", instruction, stream);
    break;
  case X86_INST_CMP:
    print_binary_instruction("cmp", instruction, stream);
    break;
  case X86_INST_JMP: {
    (void)fprintf(stream, "  jmp .L%.*s", (int)instruction.label.size,
                  instruction.label.start);
    break;
  }
  case X86_INST_JMPCC: print_cond_jmp_instruction(instruction, stream); break;
  case X86_INST_SETCC: print_cond_set_instruction(instruction, stream); break;
  case X86_INST_LABEL: {
    (void)fprintf(stream, ".L%.*s:", (int)instruction.label.size,
                  instruction.label.start);
    break;
  }
  case X86_INST_CALL:
    (void)fprintf(stream, "  call   %.*s", (int)instruction.label.size,
                  instruction.label.start);
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

  for (size_t i = 0; i < program->top_level_count; ++i) {
    const X86TopLevel top_level = program->top_levels[i];
    switch (top_level.tag) {
    case X86_TOPLEVEL_INVALID: MCC_UNREACHABLE(); break;
    case X86_TOPLEVEL_VARIABLE: {
      X86GlobalVariable* variable = top_level.variable;
      (void)fprintf(stream, ".globl %.*s\n", (int)variable->name.size,
                    variable->name.start);
      (void)fprintf(stream, ".data\n");
      (void)fprintf(stream, ".align 4\n");
      (void)fprintf(stream, "%.*s:\n", (int)variable->name.size,
                    variable->name.start);
      (void)fprintf(stream, "    .long %d\n", variable->value);
    } break;
    case X86_TOPLEVEL_FUNCTION: {
      X86FunctionDef* function = top_level.function;
      (void)fprintf(stream, ".globl %.*s\n", (int)function->name.size,
                    function->name.start);
      (void)fprintf(stream, ".type %.*s, @function\n", (int)function->name.size,
                    function->name.start);
      (void)fprintf(stream, ".text\n");
      dump_x86_function(function, stream);
    } break;
    }
  }

#ifdef __linux__
  // indicates that code does not need an execution stack
  (void)fprintf(stream, ".section .note.GNU-stack,\"\",@progbits\n");
#endif
}
