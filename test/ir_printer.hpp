#ifndef MCC_IR_PRINTER_HPP
#define MCC_IR_PRINTER_HPP

extern "C" {
#include "ir.h"
}

#include <string>

namespace mcc {

void print_to_string(std::string& buffer, IR_Instruction instr);

void print_to_string(std::string& buffer, const IR_Function& func);

void print_to_string(std::string& buffer, const IR_Module& ir_module);

} // namespace mcc

#endif // MCC_IR_PRINTER_HPP
