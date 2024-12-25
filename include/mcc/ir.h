#ifndef MCC_IR_H
#define MCC_IR_H

#include "arena.h"
#include "str.h"

// A three-address code intermediate representation

typedef struct IRProgram IRProgram;
typedef struct IRInstruction IRInstruction;
typedef struct IRFunctionDef IRFunctionDef;

typedef struct TranslationUnit TranslationUnit;

IRProgram* ir_generate(TranslationUnit* ast, Arena* permanent_arena,
                       Arena scratch_arena);
void print_ir(const IRProgram* ir);

struct IRProgram {
  size_t function_count;
  IRFunctionDef* functions;
};

struct IRFunctionDef {
  StringView name;
  size_t instruction_count;
  IRInstruction* instructions;
};

typedef enum IRValueType {
  IR_VALUE_TYPE_CONSTANT,
  IR_VALUE_TYPE_VARIABLE,
} IRValueType;

typedef struct IRValue {
  IRValueType typ;
  union {
    int32_t constant;
    StringView variable; // variable
  };
} IRValue;

typedef enum IRInstructionType {
  IR_INVALID = 0,
  IR_RETURN, // return val

  // unary
  IR_NEG,        // dest = -src
  IR_COMPLEMENT, // dest = ~src

  // binary
  IR_ADD, // dest = src1 + src2
  IR_SUB, // dest = src1 - src2
  IR_MUL, // dest = src1 * src2
  IR_DIV, // dest = src1 / src2
  IR_MOD, // dest = src1 % src2

} IRInstructionType;

struct IRInstruction {
  IRInstructionType typ;
  IRValue operand1; // often dest
  IRValue operand2;
  IRValue operand3;
};

#endif // MCC_IR_H
