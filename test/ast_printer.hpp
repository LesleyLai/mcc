#ifndef MCC_AST_PRINTER_HPP
#define MCC_AST_PRINTER_HPP

extern "C" {
#include "ast.h"
}

#include <string>

namespace mcc {

void print_to_string(std::string& buffer, const Expr& expr,
                     int indentation = 0);

void print_to_string(std::string& buffer, const Stmt& stmt,
                     int indentation = 0);

void print_to_string(std::string& buffer, const CompoundStmt& stmt,
                     int indentation = 0);

void print_to_string(std::string& buffer, const FunctionDecl& def,
                     int indentation = 0);

} // namespace mcc

#endif // MCC_AST_PRINTER_HPP
