#include <mcc/ir.h>
#include <mcc/x86.h>

#include <stdint.h>

// TODO: implement dynamic resizing
const size_t MAX_INSTRUCTION_COUNT = 160;

typedef struct X86InstructionVector {
  size_t length;
  X86Instruction* data;
} X86InstructionVector;

static X86InstructionVector new_instruction_vector(Arena* permanent_arena)
{
  return (X86InstructionVector){
      .length = 0,
      .data = ARENA_ALLOC_ARRAY(permanent_arena, X86Instruction,
                                MAX_INSTRUCTION_COUNT)};
}

static void push_instruction(X86InstructionVector* instructions,
                             X86Instruction instruction)
{
  MCC_ASSERT_MSG(instructions->length < MAX_INSTRUCTION_COUNT,
                 "Too many instructions");
  instructions->data[instructions->length++] = instruction;
}

static X86Operand x86_immediate_operand(int32_t value)
{
  return (X86Operand){.typ = X86_OPERAND_IMMEDIATE, .imm = value};
}

static X86Operand x86_register_operand(X86Register reg)
{
  return (X86Operand){.typ = X86_OPERAND_REGISTER, .reg = reg};
}

static X86Operand x86_stack_operand(size_t offset)
{
  return (X86Operand){.typ = X86_OPERAND_STACK, .stack = {.offset = offset}};
}

static X86Operand x86_operand_from_ir(IRValue ir_operand)
{
  switch (ir_operand.typ) {
  case IR_VALUE_TYPE_CONSTANT:
    return x86_immediate_operand(ir_operand.constant);
  case IR_VALUE_TYPE_VARIABLE:
    return (X86Operand){.typ = X86_OPERAND_PSEUDO,
                        .pseudo = ir_operand.variable};
  }
  MCC_UNREACHABLE();
}

static X86Instruction x86_unary_instruction(X86InstructionType type,
                                            X86Size size, X86Operand op)
{
  // TODO: verify type
  return (X86Instruction){.typ = type,
                          .unary = {
                              .size = size,
                              .op = op,
                          }};
}

static bool is_binary(X86InstructionType typ)
{
  switch (typ) {
  case x86_INST_INVALID: MCC_UNREACHABLE();
  case X86_INST_NOP:
  case X86_INST_RET:
  case X86_INST_CDQ:
  case X86_INST_NEG:
  case X86_INST_NOT:
  case X86_INST_IDIV: return false;
  case X86_INST_MOV:
  case X86_INST_ADD:
  case X86_INST_SUB:
  case X86_INST_IMUL:
  case X86_INST_AND:
  case X86_INST_OR:
  case X86_INST_XOR:
  case X86_INST_SHL:
  case X86_INST_SAR:
  case X86_INST_CMP: return true;
  case X86_INST_SETCC: return false;
  }
  MCC_UNREACHABLE();
}

static X86Instruction x86_binary_instruction(X86InstructionType type,
                                             X86Size size, X86Operand dest,
                                             X86Operand src)
{
  MCC_ASSERT_MSG(is_binary(type),
                 "Can't construct binary instruction from non-binary type");
  return (X86Instruction){.typ = type,
                          .binary = {
                              .size = size,
                              .dest = dest,
                              .src = src,
                          }};
}

// Unary instructions like neg or not
static void push_unary_instruction(X86InstructionVector* instructions,
                                   X86InstructionType type,
                                   const IRInstruction* ir_instruction)
{
  const X86Operand dst = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand src = x86_operand_from_ir(ir_instruction->operand2);

  push_instruction(instructions,
                   x86_binary_instruction(X86_INST_MOV, X86_SZ_4, dst, src));
  push_instruction(instructions, x86_unary_instruction(type, X86_SZ_4, dst));
}

static void push_binary_instruction(X86InstructionVector* instructions,
                                    X86InstructionType type,
                                    const IRInstruction* ir_instruction)
{
  const X86Operand dst = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand lhs = x86_operand_from_ir(ir_instruction->operand2);
  const X86Operand rhs = x86_operand_from_ir(ir_instruction->operand3);

  push_instruction(instructions,
                   x86_binary_instruction(X86_INST_MOV, X86_SZ_4, dst, lhs));
  push_instruction(instructions,
                   x86_binary_instruction(type, X86_SZ_4, dst, rhs));
}

static void push_div_mod_instruction(X86InstructionVector* instructions,
                                     const IRInstruction* ir_instruction)
{
  const X86Operand dst = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand lhs = x86_operand_from_ir(ir_instruction->operand2);
  const X86Operand rhs = x86_operand_from_ir(ir_instruction->operand3);

  // mov eax, <lhs>
  push_instruction(instructions, x86_binary_instruction(
                                     X86_INST_MOV, X86_SZ_4,
                                     x86_register_operand(X86_REG_AX), lhs));

  // cdq
  push_instruction(instructions, (X86Instruction){.typ = X86_INST_CDQ});

  // idiv <rhs>
  push_instruction(instructions,
                   x86_unary_instruction(X86_INST_IDIV, X86_SZ_4, rhs));

  // if div: mov <dst>, eax
  // if mod: mov <dst>, edx
  const X86Operand src = x86_register_operand(
      ir_instruction->typ == IR_DIV ? X86_REG_AX : X86_REG_DX);
  push_instruction(instructions,
                   x86_binary_instruction(X86_INST_MOV, X86_SZ_4, dst, src));
}

static void generate_comparison_instruction(X86InstructionVector* instructions,
                                            const IRInstruction* ir_instruction)
{
  const X86Operand dest = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand lhs = x86_operand_from_ir(ir_instruction->operand2);
  const X86Operand rhs = x86_operand_from_ir(ir_instruction->operand3);

  X86CondCode cond_code;
  switch (ir_instruction->typ) {
  case IR_EQUAL: cond_code = X86_COND_E; break;
  case IR_NOT_EQUAL: cond_code = X86_COND_NE; break;
  case IR_LESS: cond_code = X86_COND_L; break;
  case IR_LESS_EQUAL: cond_code = X86_COND_LE; break;
  case IR_GREATER: cond_code = X86_COND_G; break;
  case IR_GREATER_EQUAL: cond_code = X86_COND_GE; break;
  default: MCC_UNREACHABLE();
  }

  // mov dest, 0
  push_instruction(instructions,
                   x86_binary_instruction(X86_INST_MOV, X86_SZ_4, dest,
                                          x86_immediate_operand(0)));

  // cmp lhs, rhs
  push_instruction(instructions,
                   x86_binary_instruction(X86_INST_CMP, X86_SZ_4, lhs, rhs));

  // setcc dest
  push_instruction(instructions,
                   (X86Instruction){.typ = X86_INST_SETCC,
                                    .setcc = {.cond = cond_code, .op = dest}});
}

// First pass to generate assembly. Still need fixing later
static X86FunctionDef
generate_x86_function_def(const IRFunctionDef* ir_function,
                          Arena* scratch_arena)
{
  X86InstructionVector instructions = new_instruction_vector(scratch_arena);

  for (size_t j = 0; j < ir_function->instruction_count; ++j) {
    const IRInstruction* ir_instruction = &ir_function->instructions[j];
    switch (ir_instruction->typ) {
    case IR_INVALID: MCC_UNREACHABLE(); break;
    case IR_RETURN: {
      // move eax, <op>
      push_instruction(&instructions,
                       x86_binary_instruction(
                           X86_INST_MOV, X86_SZ_4,
                           x86_register_operand(X86_REG_AX),
                           x86_operand_from_ir(ir_instruction->operand1)));

      push_instruction(&instructions, (X86Instruction){.typ = X86_INST_RET});
      break;
    }

    case IR_NEG:
      push_unary_instruction(&instructions, X86_INST_NEG, ir_instruction);
      break;
    case IR_COMPLEMENT:
      push_unary_instruction(&instructions, X86_INST_NOT, ir_instruction);
      break;
    case IR_NOT: {
      X86Operand dest = x86_operand_from_ir(ir_instruction->operand1);
      X86Operand src = x86_operand_from_ir(ir_instruction->operand2);
      X86Operand zero = x86_immediate_operand(0);
      // mov dest 0
      push_instruction(&instructions, x86_binary_instruction(
                                          X86_INST_MOV, X86_SZ_4, dest, zero));

      // cmp src, 0
      push_instruction(&instructions, x86_binary_instruction(
                                          X86_INST_CMP, X86_SZ_4, src, zero));

      // sete dest
      push_instruction(
          &instructions,
          (X86Instruction){.typ = X86_INST_SETCC,
                           .setcc = {.cond = X86_COND_E, .op = dest}});
    } break;

    case IR_ADD:
      push_binary_instruction(&instructions, X86_INST_ADD, ir_instruction);
      break;
    case IR_SUB:
      push_binary_instruction(&instructions, X86_INST_SUB, ir_instruction);
      break;
    case IR_MUL:
      push_binary_instruction(&instructions, X86_INST_IMUL, ir_instruction);
      break;
    case IR_DIV:
    case IR_MOD: push_div_mod_instruction(&instructions, ir_instruction); break;
    case IR_BITWISE_AND:
      push_binary_instruction(&instructions, X86_INST_AND, ir_instruction);
      break;
    case IR_BITWISE_OR:
      push_binary_instruction(&instructions, X86_INST_OR, ir_instruction);
      break;
    case IR_BITWISE_XOR:
      push_binary_instruction(&instructions, X86_INST_XOR, ir_instruction);
      break;
    case IR_SHIFT_LEFT:
      push_binary_instruction(&instructions, X86_INST_SHL, ir_instruction);
      break;
    case IR_SHIFT_RIGHT_ARITHMETIC:
      push_binary_instruction(&instructions, X86_INST_SAR, ir_instruction);
      break;
    case IR_SHIFT_RIGHT_LOGICAL: MCC_UNIMPLEMENTED(); break;
    case IR_EQUAL:
    case IR_NOT_EQUAL:
    case IR_LESS:
    case IR_LESS_EQUAL:
    case IR_GREATER:
    case IR_GREATER_EQUAL:
      generate_comparison_instruction(&instructions, ir_instruction);
      break;
    }
  }

  return (X86FunctionDef){.name = ir_function->name,
                          .instructions = instructions.data,
                          .instruction_count = instructions.length};
}

// TODO: implement a hash table
#define MAX_UNIQUE_NAMES 16
struct UniqueNameMap {
  intptr_t count;
  StringView unique_names[MAX_UNIQUE_NAMES];
};

// Find the unique name in the map, and returns an integer index
// Returns -1 otherwise
static intptr_t try_find_unique_name(const struct UniqueNameMap* map,
                                     StringView name)
{
  for (intptr_t i = 0; i < map->count; ++i) {
    if (string_view_eq(map->unique_names[i], name)) { return i; }
  }
  return -1;
}

static size_t find_name_stack_offset(const struct UniqueNameMap* map,
                                     StringView name)
{
  const intptr_t index = try_find_unique_name(map, name);
  if (index < 0) { MCC_UNREACHABLE(); }
  return (size_t)(index + 1) * 4;
}

static void add_unique_name(struct UniqueNameMap* unique_names, StringView name)
{
  // Find whether the name is already in the map
  if (try_find_unique_name(unique_names, name) >= 0) { return; }

  MCC_ASSERT_MSG(unique_names->count - 1 < MAX_UNIQUE_NAMES,
                 "Too many unique names");

  unique_names->unique_names[unique_names->count++] = name;
}

// Add a unique name if the operand is a pseudo register
static void add_unique_name_if_pseudo(struct UniqueNameMap* unique_names,
                                      X86Operand operand)
{
  if (operand.typ == X86_OPERAND_PSEUDO) {
    add_unique_name(unique_names, operand.pseudo);
  }
}

// If operand is a pseudo register, replace it with a stack address
static void replace_pseudo_register(struct UniqueNameMap* unique_names,
                                    X86Operand* operand)
{
  if (operand->typ == X86_OPERAND_PSEUDO) {
    *operand = x86_stack_operand(
        find_name_stack_offset(unique_names, operand->pseudo));
  }
}

// Replace all pseudo-registers with stack space
// returns the stack space needed
static intptr_t replace_pseudo_registers(X86FunctionDef* function)
{
  struct UniqueNameMap unique_names = {.count = 0};

  for (size_t j = 0; j < function->instruction_count; ++j) {
    X86Instruction* instruction = &function->instructions[j];
    switch (instruction->typ) {
    case x86_INST_INVALID: MCC_UNREACHABLE(); break;
    case X86_INST_NOP:
    case X86_INST_RET:
      break;
      // Unary instruction
    case X86_INST_NEG:
    case X86_INST_NOT:
    case X86_INST_IDIV: {
      add_unique_name_if_pseudo(&unique_names, instruction->unary.op);
    } break;
      // Binary instruction
    case X86_INST_MOV:
    case X86_INST_ADD:
    case X86_INST_SUB:
    case X86_INST_IMUL:
    case X86_INST_AND:
    case X86_INST_OR:
    case X86_INST_XOR:
    case X86_INST_SHL:
    case X86_INST_SAR:
    case X86_INST_CMP: {
      add_unique_name_if_pseudo(&unique_names, instruction->binary.src);
      add_unique_name_if_pseudo(&unique_names, instruction->binary.dest);
    } break;
    case X86_INST_CDQ: break;
    case X86_INST_SETCC: {
      add_unique_name_if_pseudo(&unique_names, instruction->setcc.op);
    } break;
    }
  }

  for (size_t j = 0; j < function->instruction_count; ++j) {
    X86Instruction* instruction = &function->instructions[j];
    switch (instruction->typ) {
    case x86_INST_INVALID: MCC_UNREACHABLE(); break;
    case X86_INST_NOP:
    case X86_INST_RET: break;
    case X86_INST_NEG: // Unary operators
    case X86_INST_NOT:
    case X86_INST_IDIV: {
      replace_pseudo_register(&unique_names, &instruction->unary.op);
    } break;
    case X86_INST_MOV: // binary operators
    case X86_INST_ADD:
    case X86_INST_SUB:
    case X86_INST_IMUL:
    case X86_INST_AND:
    case X86_INST_OR:
    case X86_INST_XOR:
    case X86_INST_SHL:
    case X86_INST_SAR:
    case X86_INST_CMP: {
      replace_pseudo_register(&unique_names, &instruction->binary.src);
      replace_pseudo_register(&unique_names, &instruction->binary.dest);
    } break;
    case X86_INST_CDQ: break;
    case X86_INST_SETCC: {
      replace_pseudo_register(&unique_names, &instruction->setcc.op);
    } break;
    }
  }
  return unique_names.count * 4;
}

// Fix this binary instruction if both source and destination are memory
// addresses
static void fix_binary_instruction(X86InstructionVector* new_instructions,
                                   X86Instruction instruction)
{
  if (instruction.binary.src.typ == X86_OPERAND_STACK &&
      instruction.binary.dest.typ == X86_OPERAND_STACK) {
    const X86Operand temp_register = x86_register_operand(X86_REG_R10);
    push_instruction(
        new_instructions,
        x86_binary_instruction(X86_INST_MOV, instruction.binary.size,
                               temp_register, instruction.binary.src));

    push_instruction(
        new_instructions,
        x86_binary_instruction(instruction.typ, instruction.binary.size,
                               instruction.binary.dest, temp_register));
  } else {
    push_instruction(new_instructions, instruction);
  }
}

static void fix_shift_instruction(X86InstructionVector* new_instructions,
                                  X86Instruction instruction)
{
  if (instruction.binary.dest.typ == X86_OPERAND_STACK) {
    push_instruction(new_instructions,
                     x86_binary_instruction(X86_INST_MOV, X86_SZ_1,
                                            x86_register_operand(X86_REG_CX),
                                            instruction.binary.src));

    instruction.binary.src = x86_register_operand(X86_REG_CX);
    push_instruction(new_instructions, instruction);
  } else {
    push_instruction(new_instructions, instruction);
  }
}

static void fix_invalid_instructions(X86FunctionDef* function,
                                     intptr_t stack_size,
                                     Arena* permanent_arena)
{
  X86InstructionVector new_instructions =
      new_instruction_vector(permanent_arena);

  if (stack_size > 0) {
    push_instruction(
        &new_instructions,
        x86_binary_instruction(X86_INST_SUB, X86_SZ_8,
                               x86_register_operand(X86_REG_SP),
                               x86_immediate_operand((int)stack_size)));
  }
  for (size_t j = 0; j < function->instruction_count; ++j) {
    X86Instruction instruction = function->instructions[j];

    // Fix the situation of moving from memory to memory
    switch (instruction.typ) {
    case X86_INST_MOV:
    case X86_INST_ADD:
    case X86_INST_SUB:
    case X86_INST_AND:
    case X86_INST_OR:
    case X86_INST_XOR: {
      fix_binary_instruction(&new_instructions, instruction);
      break;
    }

      // imul can't use a memory address as its destination, regardless of its
      // source operand
    case X86_INST_IMUL: {
      if (instruction.binary.dest.typ == X86_OPERAND_STACK) {
        const X86Operand temp_register = x86_register_operand(X86_REG_R11);
        push_instruction(
            &new_instructions,
            x86_binary_instruction(X86_INST_MOV, instruction.binary.size,
                                   temp_register, instruction.binary.dest));

        push_instruction(
            &new_instructions,
            x86_binary_instruction(X86_INST_IMUL, instruction.binary.size,
                                   temp_register, instruction.binary.src));

        push_instruction(
            &new_instructions,
            x86_binary_instruction(X86_INST_MOV, instruction.binary.size,
                                   instruction.binary.dest, temp_register));
      } else {
        push_instruction(&new_instructions, instruction);
      }

      break;
    }

      // idiv can't take an immediate operand
    case X86_INST_IDIV:
      if (instruction.unary.op.typ == X86_OPERAND_IMMEDIATE) {
        const X86Operand temp_register = x86_register_operand(X86_REG_R10);
        push_instruction(
            &new_instructions,
            x86_binary_instruction(X86_INST_MOV, instruction.unary.size,
                                   temp_register, instruction.unary.op));
        push_instruction(&new_instructions,
                         x86_unary_instruction(X86_INST_IDIV,
                                               instruction.unary.size,
                                               temp_register));
      } else {
        push_instruction(&new_instructions, instruction);
      }
      break;
    case X86_INST_SHL:
    case X86_INST_SAR:
      fix_shift_instruction(&new_instructions, instruction);
      break;

    case X86_INST_CMP: {
      const X86Operand first = instruction.binary.dest;
      const X86Operand second = instruction.binary.src;

      // the first argument of cmp can't be an immediate operands
      const bool first_is_immediate = first.typ == X86_OPERAND_IMMEDIATE;
      const bool both_are_address =
          first.typ == X86_OPERAND_STACK && second.typ == X86_OPERAND_STACK;

      if (first_is_immediate || both_are_address) {
        const X86Operand temp_register = x86_register_operand(X86_REG_R10);
        const X86Size size = instruction.binary.size;

        push_instruction(
            &new_instructions,
            x86_binary_instruction(X86_INST_MOV, size, temp_register, first));
        push_instruction(
            &new_instructions,
            x86_binary_instruction(X86_INST_CMP, size, temp_register, second));
      } else {
        push_instruction(&new_instructions, instruction);
      }
    } break;
    default: push_instruction(&new_instructions, instruction);
    }
  }

  function->instruction_count = new_instructions.length;
  function->instructions = new_instructions.data;
}

static X86FunctionDef x86_generate_function(const IRFunctionDef* ir_function,
                                            Arena* permanent_arena,
                                            Arena scratch_arena)
{
  // passes to generate an x86 assembly function
  X86FunctionDef function =
      generate_x86_function_def(ir_function, &scratch_arena);
  const intptr_t stack_size = replace_pseudo_registers(&function);
  fix_invalid_instructions(&function, stack_size, permanent_arena);

  return function;
}

X86Program x86_generate_assembly(IRProgram* ir, Arena* permanent_arena,
                                 Arena scratch_arena)
{
  const size_t function_count = ir->function_count;
  X86FunctionDef* functions =
      ARENA_ALLOC_ARRAY(permanent_arena, X86FunctionDef, function_count);
  for (size_t i = 0; i < function_count; ++i) {
    functions[i] = x86_generate_function(&ir->functions[i], permanent_arena,
                                         scratch_arena);
  }

  return (X86Program){.function_count = function_count, .functions = functions};
}
