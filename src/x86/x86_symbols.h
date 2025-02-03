#ifndef MCC_X86_SYMBOLS_H
#define MCC_X86_SYMBOLS_H

#include <mcc/arena.h>
#include <mcc/str.h>

typedef struct Symbols Symbols;

Symbols* new_symbol_table(Arena* arena);

bool has_symbol(const Symbols* symbols, StringView name);

void add_symbol(Symbols* symbols, StringView name, Arena* arena);

#endif // MCC_X86_SYMBOLS_H
