#include <mcc/ir.h>
#include <mcc/x86.h>

#include <stdint.h>

#include "x86_passes.h"
#include "x86_symbols.h"

static X86FunctionDef x86_generate_function(const IRFunctionDef* ir_function,
                                            X86CodegenContext* context)
{
  // passes to generate an x86 assembly function
  X86FunctionDef function = x86_function_from_ir(ir_function, context);
  const uint32_t stack_size = replace_pseudo_registers(&function, context);
  fix_invalid_instructions(&function, stack_size, context);

  return function;
}

X86Program x86_generate_assembly(IRProgram* ir, Arena* permanent_arena,
                                 Arena scratch_arena)
{
  const size_t function_count = ir->function_count;
  X86FunctionDef* functions =
      ARENA_ALLOC_ARRAY(permanent_arena, X86FunctionDef, function_count);

  X86CodegenContext context = {
      .permanent_arena = permanent_arena,
      .scratch_arena = scratch_arena,
      .symbols = new_symbol_table(permanent_arena),
  };

  for (size_t i = 0; i < function_count; ++i) {
    functions[i] = x86_generate_function(&ir->functions[i], &context);
  }

  return (X86Program){.function_count = function_count, .functions = functions};
}
