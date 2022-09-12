#ifndef MCC_IR_H
#define MCC_IR_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "utils/str.h"

// mcc first transform AST nodes into IR, and then generate machine code from it
// In the future, I will probably also add some optimizations in this phase
// The IR in mcc is three-address code with "infinite" registers.
// Each instruction is a tuple (opcode, r1, r2, r3) Even though not all
// instructions use all registers

enum IR_Opcode {
  IR_NOP,
  IR_RETURN,

  IR_LOAD_LITERAL,

  // Binary operators
  IR_ADD,
  IR_SUB,
  IR_MUL,
  IR_DIV,
};

enum IR_OpType { IR_Type_I32 };

// The index of a register
typedef uint32_t Register;

typedef struct IR_Instruction {
  uint8_t opcode;
  uint8_t optype;

  union {
    struct {
      Register rdest;
    } return_i32;

    struct {
      int32_t value;
      Register rdest;
    } load_i32_literal;

    struct {
      Register r1;
      Register r2;
      Register rdest;
    } binary_op;
  } data;
} IR_Instruction;

typedef struct IR_Function {
  StringBuffer name;

  // TODO: use a vector
  size_t size;
  IR_Instruction* instructions;

  uint32_t register_count;
} IR_Function;

/**
 * @brief An IR Module corresponding to a translation unit in C
 */
typedef struct IR_Module {
  // TODO: use a vector
  size_t function_count;
  IR_Function* functions;
} IR_Module;

static_assert(sizeof(IR_Instruction) == 16,
              "An instruction is always 16 bytes");

#endif // MCC_IR_H
