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
      x86_function_from_ir(ir_function, permanent_arena, &scratch_arena);
  const intptr_t stack_size =
      replace_pseudo_registers(&function, permanent_arena);
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
