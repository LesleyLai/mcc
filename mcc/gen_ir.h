#ifndef MCC_GEN_IR_H
#define MCC_GEN_IR_H

#include "ast.h"
#include "ir.h"
#include "utils/allocators.h"

IR_Module* generate_ir(TranslationUnit* tu, PolyAllocator* allocator);

#endif // MCC_GEN_IR_H
