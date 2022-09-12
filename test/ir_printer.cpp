#include "ir_printer.hpp"

#include <fmt/format.h>

#include "cstring_view_format.hpp"

namespace mcc {

void print_to_string(std::string& buffer, IR_Instruction instr)
{
  switch (instr.opcode) {
  case IR_NOP: fmt::format_to(std::back_inserter(buffer), "nop\n"); return;
  case IR_LOAD_LITERAL:
    fmt::format_to(std::back_inserter(buffer), "r{}: i32 = {}\n",
                   instr.data.load_i32_literal.rdest,
                   instr.data.load_i32_literal.value);
    return;

  case IR_RETURN:
    fmt::format_to(std::back_inserter(buffer), "return i32 r{}\n",
                   instr.data.return_i32.rdest);
    return;

  case IR_ADD:
    fmt::format_to(std::back_inserter(buffer), "r{} = r{} + r{}\n",
                   instr.data.binary_op.rdest, instr.data.binary_op.r1,
                   instr.data.binary_op.r2);
    return;
  case IR_SUB:
    fmt::format_to(std::back_inserter(buffer), "r{} = r{} - r{}\n",
                   instr.data.binary_op.rdest, instr.data.binary_op.r1,
                   instr.data.binary_op.r2);
    return;
  case IR_MUL:
    fmt::format_to(std::back_inserter(buffer), "r{} = r{} * r{}\n",
                   instr.data.binary_op.rdest, instr.data.binary_op.r1,
                   instr.data.binary_op.r2);
    return;
  case IR_DIV:
    fmt::format_to(std::back_inserter(buffer), "r{} = r{} / r{}\n",
                   instr.data.binary_op.rdest, instr.data.binary_op.r1,
                   instr.data.binary_op.r2);
    return;
  }

  fmt::format_to(std::back_inserter(buffer), "Unknown opcode {}\n",
                 instr.opcode);
}

void print_to_string(std::string& buffer, const IR_Function& func)
{
  fmt::format_to(std::back_inserter(buffer), "func {}\n",
                 string_view_from_buffer(&func.name));

  for (std::size_t i = 0; i < func.size; ++i) {
    fmt::format_to(std::back_inserter(buffer), "    ");
    print_to_string(buffer, func.instructions[i]);
  }
}

void print_to_string(std::string& buffer, const IR_Module& ir_module)
{
  for (std::size_t i = 0; i < ir_module.function_count; ++i) {
    print_to_string(buffer, ir_module.functions[i]);
    if (i < ir_module.function_count - 1) { buffer.push_back('\n'); }
  }
}

} // namespace mcc