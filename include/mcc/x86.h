#ifndef MCC_X86_H
#define MCC_X86_H

#include "arena.h"
#include "hash_table.h"
#include "str.h"

typedef struct X86Program X86Program;
typedef struct X86TopLevel X86TopLevel;
typedef struct X86GlobalVariable X86GlobalVariable;
typedef struct X86FunctionDef X86FunctionDef;
typedef struct X86Instruction X86Instruction;
typedef struct X86Operand X86Operand;

struct X86Program {
  size_t top_level_count;
  X86TopLevel* top_levels;
};

typedef enum X86TopLevelTag {
  X86_TOPLEVEL_INVALID = 0,
  X86_TOPLEVEL_VARIABLE,
  X86_TOPLEVEL_FUNCTION,
} X86TopLevelTag;

struct X86TopLevel {
  X86TopLevelTag tag;
  union {
    X86FunctionDef* function;
    X86GlobalVariable* variable;
  };
};

struct X86FunctionDef {
  StringView name;
  size_t instruction_count;
  X86Instruction* instructions;
};

struct X86GlobalVariable {
  StringView name;
  int32_t value;
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
  X86_REG_DI,
  X86_REG_SI,
  X86_REG_R8,
  X86_REG_R9,
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
  X86_OPERAND_DATA,   // Global variable
} X86OperandType;

struct X86StackOperand {
  intptr_t offset;
};

struct X86Operand {
  X86OperandType typ;
  union {
    int32_t imm;
    X86Register reg;
    StringView pseudo;
    struct X86StackOperand stack;
    StringView data;
  };
};

typedef enum X86InstructionType {
  x86_INST_INVALID = 0,

  // instructions without payload
  X86_INST_NOP,
  X86_INST_RET,
  X86_INST_CDQ, // extends the sign bit of eax into the edx register

  // unary instructions
  X86_INST_NEG, // negation
  X86_INST_NOT, // bitwise complement
  X86_INST_IDIV,
  X86_INST_PUSH,

  // binary instructions
  X86_INST_MOV,
  X86_INST_ADD,
  X86_INST_SUB,
  X86_INST_IMUL,
  X86_INST_AND, // bitwise and
  X86_INST_OR,  // bitwise or
  X86_INST_XOR, // bitwise xor
  X86_INST_SHL, // Left Shift
  X86_INST_SAR, // Arithmetic Right Shift
  X86_INST_CMP, // Compare

  // jump related instructions
  X86_INST_JMP,   // unconditional jump
  X86_INST_JMPCC, // conditional jump
  X86_INST_SETCC,
  X86_INST_LABEL,
  X86_INST_CALL,
} X86InstructionType;

typedef enum X86CondCode {
  X86_COND_INVALID = 0,
  X86_COND_E,  // equal
  X86_COND_NE, // not equal
  X86_COND_G,  // greater tjam
  X86_COND_GE, // greater than or equal
  X86_COND_L,  // less than
  X86_COND_LE, // less than or equal
} X86CondCode;

struct X86Instruction {
  X86InstructionType typ;
  union {
    struct {
      X86Size size;
      X86Operand op;
    } unary;
    struct {
      X86Size size;
      X86Operand dest;
      X86Operand src;
    } binary;
    struct {
      X86CondCode cond;
      StringView label;
    } jmpcc;
    struct {
      X86CondCode cond;
      X86Operand op;
    } setcc;
    StringView label; // label, jmp, or call
  };
};

struct IRProgram;

X86Program x86_generate_assembly(struct IRProgram* ir, Arena* permanent_arena,
                                 Arena scratch_arena);

void x86_dump_assembly(const X86Program* program, FILE* stream);
void x86_print_instruction(X86Instruction instruction, FILE* stream);

#endif // MCC_X86_H
