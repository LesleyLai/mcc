#include "x86_passes.h"

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
intptr_t replace_pseudo_registers(X86FunctionDef* function)
{
  struct UniqueNameMap unique_names = {.count = 0};

  for (size_t j = 0; j < function->instruction_count; ++j) {
    X86Instruction* instruction = &function->instructions[j];
    switch (instruction->typ) {
    case x86_INST_INVALID: MCC_UNREACHABLE(); break;
    case X86_INST_NOP:
    case X86_INST_RET:
      break;
    X86_UNARY_INSTRUCTION_CASES: {
      add_unique_name_if_pseudo(&unique_names, instruction->unary.op);
    } break;
    X86_BINARY_INSTRUCTION_CASES: {
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
    case X86_INST_RET:
      break;
    X86_UNARY_INSTRUCTION_CASES: {
      replace_pseudo_register(&unique_names, &instruction->unary.op);
    } break;
    X86_BINARY_INSTRUCTION_CASES: {
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
