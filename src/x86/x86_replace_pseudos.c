#include "x86_passes.h"

#include <mcc/dynarray.h>

// TODO: implement a hash table
struct UniqueNameMap {
  size_t length;
  size_t capacity;
  StringView* data;
  Arena* arena;
};

// Find the unique name in the map, and returns an integer index
// Returns -1 otherwise
static intptr_t try_find_unique_name(const struct UniqueNameMap* map,
                                     StringView name)
{
  for (size_t i = 0; i < map->length; ++i) {
    if (str_eq(map->data[i], name)) { return (intptr_t)i; }
  }
  return -1;
}

static intptr_t find_name_stack_offset(const struct UniqueNameMap* map,
                                       StringView name)
{
  const intptr_t index = try_find_unique_name(map, name);
  if (index < 0) { MCC_UNREACHABLE(); }
  return (index + 1) * 4;
}

static void add_unique_name(struct UniqueNameMap* unique_names, StringView name)
{
  // Find whether the name is already in the map
  if (try_find_unique_name(unique_names, name) >= 0) { return; }

  DYNARRAY_PUSH_BACK(unique_names, StringView, unique_names->arena, name);
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
    *operand =
        stack_operand(find_name_stack_offset(unique_names, operand->pseudo));
  }
}

// Replace all pseudo-registers with stack space
// returns the stack space needed
uint32_t replace_pseudo_registers(X86InstructionVector* instructions,
                                  X86CodegenContext* context)
{
  struct UniqueNameMap unique_names = {.arena = context->permanent_arena};

  for (size_t i = 0; i < instructions->length; ++i) {
    X86Instruction* instruction = &instructions->data[i];
    switch (instruction->typ) {
    case x86_INST_INVALID:
      MCC_UNREACHABLE();
      break;
    X86_UNARY_INSTRUCTION_CASES:
      add_unique_name_if_pseudo(&unique_names, instruction->unary.op);
      break;
    X86_BINARY_INSTRUCTION_CASES:
      add_unique_name_if_pseudo(&unique_names, instruction->binary.src);
      add_unique_name_if_pseudo(&unique_names, instruction->binary.dest);
      break;
    case X86_INST_SETCC:
      add_unique_name_if_pseudo(&unique_names, instruction->setcc.op);
      break;
    case X86_INST_NOP: [[fallthrough]];
    case X86_INST_RET: [[fallthrough]];
    case X86_INST_CDQ: [[fallthrough]];
    case X86_INST_JMP: [[fallthrough]];
    case X86_INST_JMPCC: [[fallthrough]];
    case X86_INST_LABEL: [[fallthrough]];
    case X86_INST_CALL: break;
    }
  }

  for (size_t j = 0; j < instructions->length; ++j) {
    X86Instruction* instruction = &instructions->data[j];
    switch (instruction->typ) {
    case x86_INST_INVALID:
      MCC_UNREACHABLE();
      break;
    X86_UNARY_INSTRUCTION_CASES: {
      replace_pseudo_register(&unique_names, &instruction->unary.op);
    } break;
    X86_BINARY_INSTRUCTION_CASES: {
      replace_pseudo_register(&unique_names, &instruction->binary.src);
      replace_pseudo_register(&unique_names, &instruction->binary.dest);
    } break;
    case X86_INST_SETCC: {
      replace_pseudo_register(&unique_names, &instruction->setcc.op);
    } break;
    case X86_INST_NOP: [[fallthrough]];
    case X86_INST_RET: [[fallthrough]];
    case X86_INST_CDQ: [[fallthrough]];
    case X86_INST_JMP: [[fallthrough]];
    case X86_INST_JMPCC: [[fallthrough]];
    case X86_INST_LABEL: [[fallthrough]];
    case X86_INST_CALL: break;
    }
  }
  uint32_t stack_space = u32_from_usize(unique_names.length * 4);
  stack_space =
      (stack_space + 15) & ~15u; // round the stack space to multiple of 16
  return stack_space;
}
