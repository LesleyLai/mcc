#ifndef MCC_IR_H
#define MCC_IR_H

#include <stddef.h>

#include "utils/str.h"

#include "ast.h"

// A three-address code intermediate representation

typedef struct IRProgram IRProgram;
typedef struct IRInstruction IRInstruction;
typedef struct IRFunctionDef IRFunctionDef;

IRProgram* generate_ir(TranslationUnit* ast, Arena* permanent_arena,
                       Arena scratch_arena);
void dump_ir(const IRProgram* ir);

struct IRProgram {
  size_t size;
  IRFunctionDef* functions;
};

struct IRFunctionDef {
  StringView name;
  size_t instruction_count;
  IRInstruction* instructions;
};

typedef enum IRValueType {
  IR_VALUE_TYPE_INVALID = 0,
  IR_VALUE_TYPE_CONSTANT,
  IR_VALUE_TYPE_VARIABLE,
} IRValueType;

typedef struct IRValue {
  IRValueType typ;
  union {
    int constant;
    StringView name; // variable
  };
} IRValue;

typedef enum IRInstructionType {
  IR_NEG,        // dest = -src
  IR_COMPLEMENT, // dest = ~src
  IR_RETURN,     // return val
} IRInstructionType;

struct IRInstruction {
  IRInstructionType typ;
  IRValue value1; // often dest
  IRValue value2;
  IRValue value3;
};

/*
 * =============================================================================
 * Convenient "constructors"
 * =============================================================================
 */
static inline IRInstruction ir_single_operand_instr(IRInstructionType typ,
                                                    IRValue operand)
{
  return MCC_COMPOUND_LITERAL(IRInstruction){.typ = typ, .value1 = operand};
}

static inline IRInstruction ir_unary_instr(IRInstructionType typ, IRValue dst,
                                           IRValue src)
{
  return MCC_COMPOUND_LITERAL(IRInstruction){
      .typ = typ, .value1 = dst, .value2 = src};
}

static inline IRValue ir_variable(StringView name)
{
  return MCC_COMPOUND_LITERAL(IRValue){.typ = IR_VALUE_TYPE_VARIABLE,
                                       .name = name};
}

#endif // MCC_IR_H
