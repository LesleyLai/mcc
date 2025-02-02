#ifndef MCC_X86_PASSES_H
#define MCC_X86_PASSES_H

#include <mcc/x86.h>

#include "x86_helpers.h"

struct IRFunctionDef;

/// @brief Converts an IR function into an x86 function.
///
/// This is the first pass in the x86 generation process.
/// @note At this stage, the generated x86 function may still contain invalid
/// instructions, as these are resolved in subsequent passes.
X86FunctionDef x86_function_from_ir(const struct IRFunctionDef* ir_function,
                                    Arena* permanent_arena,
                                    Arena* scratch_arena);

/// @brief Replaces all pseudo-registers in the x86 function with allocated
/// stack space.
///
/// This is the second pass in the x86 generation process. It calculates the
/// required stack size for the function and updates the instructions to use
/// stack slots instead of pseudo-registers.
///
/// @param[inout] function Pointer to the x86 function whose pseudo-registers
/// will be replaced.
/// @return The size of the stack needed for the function in bytes.
intptr_t replace_pseudo_registers(X86FunctionDef* function,
                                  Arena* permanent_arena);

/// @brief Resolves invalid x86 instructions in the function.
///
/// This is the final pass in the x86 generation process. It corrects invalid
/// instructions, such as operations involving multiple memory addresses in a
/// single instruction. This pass ensures the function adheres to valid x86
/// instruction formats.
///
/// @param[inout] function Pointer to the x86 function to be fixed.
/// @param stack_size The size of the stack allocated for the function.
/// @param[inout] permanent_arena Permanent memory arena used for storing the
/// final function definition.
///
void fix_invalid_instructions(X86FunctionDef* function, intptr_t stack_size,
                              Arena* permanent_arena);

#endif // MCC_X86_PASSES_H
