#ifndef MCC_X86_H
#define MCC_X86_H

#include "arena.h"
#include "str.h"

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

// Size in bytes
typedef enum X86Size {
  X86_SZ_1 = 1,
  X86_SZ_2 = 2,
  X86_SZ_4 = 4,
  X86_SZ_8 = 8,
} X86Size;

typedef enum X86Register {
  X86_REG_INVALID = 0,
  X86_REG_AX,
  X86_REG_BX,
  X86_REG_CX,
  X86_REG_DX,
  X86_REG_R10,
  X86_REG_R11,
  X86_REG_SP, // Stack pointer
} X86Register;

typedef enum X86OperandType {
  X86_OPERAND_INVALID = 0,
  X86_OPERAND_IMMEDIATE,
  X86_OPERAND_REGISTER,
  X86_OPERAND_PSEUDO, // Represent a temporary variable
  X86_OPERAND_STACK,  // Something on the stack
} X86OperandType;

struct X86StackOperand {
  size_t offset;
};

struct X86Operand {
  X86OperandType typ;
  union {
    int32_t imm;
    X86Register reg;
    StringView pseudo;
    struct X86StackOperand stack;
  };
};

typedef enum X86InstructionType {
  x86_INST_INVALID = 0,
  X86_INST_NOP,
  X86_INST_RET,
  X86_INST_MOV,

  X86_INST_NEG,
  X86_INST_NOT,

  X86_INST_ADD,
  X86_INST_SUB,
  X86_INST_IMUL,
  X86_INST_IDIV,

  X86_INST_AND, // bitwise and
  X86_INST_OR,  // bitwise or
  X86_INST_XOR, // bitwise xor
  X86_INST_SHL, // Left Shift
  X86_INST_SAR, // Arithemtic Right Shift

  X86_INST_CDQ, // extends the sign bit of eax into the edx register

  X86_INST_CMP, // Compare

  // conditional set instructions
  X86_INST_SETE,
  X86_INST_SETNE,
  X86_INST_SETG,
  X86_INST_SETGE,
  X86_INST_SETL,
  X86_INST_SETLE,
} X86InstructionType;

struct X86Instruction {
  X86InstructionType typ;
  X86Size size;        // size in bytes
  X86Operand operand1; // Usually destination (in Intel syntax)
  X86Operand operand2;
};

struct IRProgram;

X86Program x86_generate_assembly(struct IRProgram* ir, Arena* permanent_arena,
                                 Arena scratch_arena);

void x86_dump_assembly(const X86Program* program, FILE* stream);
void x86_print_instruction(X86Instruction instruction, FILE* stream);

#endif // MCC_X86_H
