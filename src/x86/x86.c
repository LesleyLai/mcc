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
  const size_t top_level_count = ir->top_level_count;
  X86TopLevel* top_levels =
      ARENA_ALLOC_ARRAY(permanent_arena, X86TopLevel, top_level_count);

  X86CodegenContext context = {
      .permanent_arena = permanent_arena,
      .scratch_arena = scratch_arena,
      .symbols = new_symbol_table(permanent_arena),
  };

  for (size_t i = 0; i < top_level_count; ++i) {
    switch (ir->top_levels[i]->tag) {
    case IR_TOP_LEVEL_INVALID: MCC_UNREACHABLE(); break;
    case IR_TOP_LEVEL_FUNCTION: {
      X86FunctionDef* function =
          ARENA_ALLOC_OBJECT(permanent_arena, X86FunctionDef);
      *function = x86_generate_function(&ir->top_levels[i]->function, &context);

      top_levels[i] = (X86TopLevel){
          .tag = X86_TOPLEVEL_FUNCTION,
          .function = function,
      };
    } break;
    case IR_TOP_LEVEL_VARIABLE: {
      const IRGlobalVariable ir_variable = ir->top_levels[i]->variable;

      X86GlobalVariable* variable =
          ARENA_ALLOC_OBJECT(permanent_arena, X86GlobalVariable);
      *variable = (X86GlobalVariable){
          .name = ir_variable.name,
          .value = ir_variable.value,
      };

      add_symbol(context.symbols, variable->name, context.permanent_arena);

      top_levels[i] = (X86TopLevel){
          .tag = X86_TOPLEVEL_VARIABLE,
          .variable = variable,
      };
    } break;
    }
  }

  return (X86Program){.top_level_count = top_level_count,
                      .top_levels = top_levels};
}
