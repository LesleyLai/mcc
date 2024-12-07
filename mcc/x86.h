#ifndef MCC_X86_H
#define MCC_X86_H

#include "utils/arena.h"
#include "utils/str.h"
#include <stddef.h>
#include <stdint.h>

typedef struct X86Program X86Program;
typedef struct X86FunctionDef X86FunctionDef;
typedef struct X86Instruction X86Instruction;
typedef struct X86Operand X86Operand;

struct X86Program {
  size_t function_count;
  X86FunctionDef* functions;
};

struct X86FunctionDef {
  StringView name;
  size_t instruction_count;
  X86Instruction* instructions;
};

typedef enum X86OperandType {
  X86_OPERAND_IMMEDIATE,
  X86_OPERAND_REGISTER,
} X86OperandType;

struct X86Operand {
  X86OperandType typ;
  union {
    int32_t imm;
  };
};

typedef enum X86InstructionType {
  X86_INST_NOP,
  X86_INST_RET,
  X86_INST_MOV,
} X86InstructionType;

struct X86Instruction {
  X86InstructionType typ;
  X86Operand operand1; // Usually destination (in Intel syntax)
  X86Operand operand2;
};

struct IRProgram;
X86Program generate_x86_assembly(struct IRProgram* ir, Arena* permanent_arena);

void dump_x86_assembly(const X86Program* program, FILE* stream);

#endif // MCC_X86_H
