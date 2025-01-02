#ifndef MCC_X86_PASSES_H
#define MCC_X86_PASSES_H

#include <mcc/x86.h>

#include "x86_helpers.h"

struct IRFunctionDef;

X86FunctionDef
generate_x86_function_def(const struct IRFunctionDef* ir_function,
                          Arena* scratch_arena);

intptr_t replace_pseudo_registers(X86FunctionDef* function);

X86InstructionVector fix_invalid_instructions(X86Instruction* instructions,
                                              size_t instruction_count,
                                              intptr_t stack_size,
                                              Arena* permanent_arena);

#endif // MCC_X86_PASSES_H
