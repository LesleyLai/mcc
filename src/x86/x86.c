#include <mcc/ir.h>
#include <mcc/x86.h>

#include <stdint.h>

#include "x86_passes.h"

static X86FunctionDef x86_generate_function(const IRFunctionDef* ir_function,
                                            Arena* permanent_arena,
                                            Arena scratch_arena)
{
  // passes to generate an x86 assembly function
  X86FunctionDef function =
      generate_x86_function_def(ir_function, &scratch_arena);
  const intptr_t stack_size = replace_pseudo_registers(&function);
  const X86InstructionVector fixed_instructions = fix_invalid_instructions(
      function.instructions, function.instruction_count, stack_size,
      permanent_arena);
  function.instruction_count = fixed_instructions.length;
  function.instructions = fixed_instructions.data;

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
