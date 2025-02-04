#include <mcc/ir.h>
#include <mcc/x86.h>

#include <stdint.h>
#include <string.h>

#include "x86_passes.h"
#include "x86_symbols.h"

static X86FunctionDef x86_generate_function(const IRFunctionDef* ir_function,
                                            X86CodegenContext* context)
{
  const Arena old_scratch_arena = context->scratch_arena;

  // passes to generate an x86 assembly function
  X86InstructionVector instructions =
      x86_from_ir_function(ir_function, context);
  const uint32_t stack_size = replace_pseudo_registers(&instructions, context);
  X86InstructionVector fixed_instructions =
      fix_invalid_instructions(&instructions, stack_size, context);

  // Copy the final instruction result to permanent buffer
  X86Instruction* instruction_buffer = ARENA_ALLOC_ARRAY(
      context->permanent_arena, X86Instruction, fixed_instructions.length);
  if (fixed_instructions.length != 0) {
    memcpy(instruction_buffer, fixed_instructions.data,
           fixed_instructions.length * sizeof(X86Instruction));
  }

  // Free memory of the scratch arena across different functions
  context->scratch_arena = old_scratch_arena;

  return (X86FunctionDef){
      .name = ir_function->name,
      .instructions = instruction_buffer,
      .instruction_count = fixed_instructions.length,
  };
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
