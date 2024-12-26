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

// Unary instructions like neg or not
static void push_unary_instruction(X86InstructionVector* instructions,
                                   X86InstructionType type,
                                   const IRInstruction* ir_instruction)
{
  const X86Operand dst = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand src = x86_operand_from_ir(ir_instruction->operand2);

  push_instruction(instructions, (X86Instruction){
                                     .typ = X86_INST_MOV,
                                     .operand1 = dst,
                                     .operand2 = src,
                                 });

  push_instruction(instructions, (X86Instruction){
                                     .typ = type,
                                     .operand1 = dst,
                                 });
}

static void push_binary_instruction(X86InstructionVector* instructions,
                                    X86InstructionType type,
                                    const IRInstruction* ir_instruction)
{
  const X86Operand dst = x86_operand_from_ir(ir_instruction->operand1);
  const X86Operand lhs = x86_operand_from_ir(ir_instruction->operand2);
  const X86Operand rhs = x86_operand_from_ir(ir_instruction->operand3);

  push_instruction(
      instructions,
      (X86Instruction){.typ = X86_INST_MOV, .operand1 = dst, .operand2 = lhs});
  push_instruction(
      instructions,
      (X86Instruction){.typ = type, .operand1 = dst, .operand2 = rhs});
}

static void push_div_mod_instruction(X86InstructionVector* instructions,
                                     const IRInstruction* ir_instruction)
{
  // mov eax, <lhs>
  push_instruction(
      instructions,
      (X86Instruction){
          .typ = X86_INST_MOV,
          .operand1 = x86_register_operand(X86_REG_AX),
          .operand2 = x86_operand_from_ir(ir_instruction->operand2),
      });

  // cdq
  push_instruction(instructions, (X86Instruction){.typ = X86_INST_CDQ});

  // idiv <rhs>
  push_instruction(instructions,
                   (X86Instruction){.typ = X86_INST_IDIV,
                                    .operand1 = x86_operand_from_ir(
                                        ir_instruction->operand3)});

  // if div: mov <dst>, eax
  // if mod: mov <dst>, edx
  X86Operand src;
  switch (ir_instruction->typ) {
  case IR_DIV: src = x86_register_operand(X86_REG_AX); break;
  case IR_MOD: src = x86_register_operand(X86_REG_DX); break;
  default: MCC_UNREACHABLE();
  }

  push_instruction(instructions, (X86Instruction){
                                     .typ = X86_INST_MOV,
                                     .operand1 = x86_operand_from_ir(
                                         ir_instruction->operand1),
                                     .operand2 = src,
                                 });
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
      push_instruction(
          &instructions,
          (X86Instruction){
              .typ = X86_INST_MOV,
              .operand1 = x86_register_operand(X86_REG_AX),
              .operand2 = x86_operand_from_ir(ir_instruction->operand1),
          });

      push_instruction(&instructions, (X86Instruction){.typ = X86_INST_RET});
      break;
    }

    case IR_NEG:
      push_unary_instruction(&instructions, X86_INST_NEG, ir_instruction);
      break;
    case IR_COMPLEMENT:
      push_unary_instruction(&instructions, X86_INST_NOT, ir_instruction);
      break;
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

static void add_unique_name(struct UniqueNameMap* map, StringView name)
{
  // Find whether the name is already in the map
  if (try_find_unique_name(map, name) >= 0) { return; }

  MCC_ASSERT_MSG(map->count - 1 < MAX_UNIQUE_NAMES, "Too many unique names");

  map->unique_names[map->count++] = name;
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
      if (instruction->operand1.typ == X86_OPERAND_PSEUDO) {
        add_unique_name(&unique_names, instruction->operand1.pseudo);
      }
    } break;
      // Binary instruction
    case X86_INST_MOV:
    case X86_INST_ADD:
    case X86_INST_SUB:
    case X86_INST_IMUL: {
      if (instruction->operand1.typ == X86_OPERAND_PSEUDO) {
        add_unique_name(&unique_names, instruction->operand1.pseudo);
      }
      if (instruction->operand2.typ == X86_OPERAND_PSEUDO) {
        add_unique_name(&unique_names, instruction->operand2.pseudo);
      }
    } break;
    case X86_INST_CDQ: break;
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
      if (instruction->operand1.typ == X86_OPERAND_PSEUDO) {
        instruction->operand1 = x86_stack_operand(find_name_stack_offset(
            &unique_names, instruction->operand1.pseudo));
      }
    } break;
    case X86_INST_MOV: // binary operators
    case X86_INST_ADD:
    case X86_INST_SUB:
    case X86_INST_IMUL: {
      if (instruction->operand1.typ == X86_OPERAND_PSEUDO) {
        instruction->operand1 = x86_stack_operand(find_name_stack_offset(
            &unique_names, instruction->operand1.pseudo));
      }
      if (instruction->operand2.typ == X86_OPERAND_PSEUDO) {
        instruction->operand2 = x86_stack_operand(find_name_stack_offset(
            &unique_names, instruction->operand2.pseudo));
      }
    } break;
    case X86_INST_CDQ: break;
    }
  }
  return unique_names.count * 4;
}

// Fix this binary instruction if both source and destination are memory
// addresses
static void fix_binary_operands(X86InstructionVector* new_instructions,
                                X86Instruction instruction)
{
  if (instruction.operand1.typ == X86_OPERAND_STACK &&
      instruction.operand2.typ == X86_OPERAND_STACK) {
    push_instruction(
        new_instructions,
        (X86Instruction){.typ = X86_INST_MOV,
                         .operand1 = x86_register_operand(X86_REG_R10),
                         .operand2 = instruction.operand2});

    push_instruction(
        new_instructions,
        (X86Instruction){.typ = instruction.typ,
                         .operand1 = instruction.operand1,
                         .operand2 = x86_register_operand(X86_REG_R10)});
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
        (X86Instruction){.typ = X86_INST_SUB,
                         .operand1 = x86_register_operand(X86_REG_SP),
                         // TODO: properly handle different size of operands
                         .operand2 = x86_immediate_operand((int)stack_size)});
  }
  for (size_t j = 0; j < function->instruction_count; ++j) {
    X86Instruction instruction = function->instructions[j];

    // Fix the situation of moving from memory to memory
    switch (instruction.typ) {
    case X86_INST_MOV:
    case X86_INST_ADD:
    case X86_INST_SUB: {
      fix_binary_operands(&new_instructions, instruction);
      break;
    }

      // imul can't use a memory address as its destination, regardless of its
      // source operand
    case X86_INST_IMUL: {
      if (instruction.operand1.typ == X86_OPERAND_STACK) {
        push_instruction(
            &new_instructions,
            (X86Instruction){.typ = X86_INST_MOV,
                             .operand1 = x86_register_operand(X86_REG_R11),
                             .operand2 = instruction.operand1});
        push_instruction(
            &new_instructions,
            (X86Instruction){.typ = X86_INST_IMUL,
                             .operand1 = x86_register_operand(X86_REG_R11),
                             .operand2 = instruction.operand2});
        push_instruction(
            &new_instructions,
            (X86Instruction){.typ = X86_INST_MOV,
                             .operand1 = instruction.operand1,
                             .operand2 = x86_register_operand(X86_REG_R11)});
      } else {
        push_instruction(&new_instructions, instruction);
      }

      break;
    }

      // idiv can't take an immediate operand
    case X86_INST_IDIV:
      if (instruction.operand1.typ == X86_OPERAND_IMMEDIATE) {
        push_instruction(
            &new_instructions,
            (X86Instruction){.typ = X86_INST_MOV,
                             .operand1 = x86_register_operand(X86_REG_R10),
                             .operand2 = instruction.operand1});
        push_instruction(
            &new_instructions,
            (X86Instruction){.typ = X86_INST_IDIV,
                             .operand1 = x86_register_operand(X86_REG_R10)});
      } else {
        push_instruction(&new_instructions, instruction);
      }
      break;
    default: push_instruction(&new_instructions, function->instructions[j]);
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
