#include "x86_symbols.h"

#include <mcc/dynarray.h>

struct Symbols {
  uint32_t length;
  uint32_t capacity;
  StringView* data;
};

Symbols* new_symbol_table(Arena* arena)
{
  return ARENA_ALLOC_OBJECT(arena, Symbols);
}

bool has_symbol(const Symbols* symbols, StringView name)
{
  for (uint32_t i = 0; i < symbols->length; ++i) {
    if (str_eq(name, symbols->data[i])) return true;
  }
  return false;
}

void add_symbol(Symbols* symbols, StringView name, Arena* arena)
{
  MCC_ASSERT(!has_symbol(symbols, name));
  DYNARRAY_PUSH_BACK(symbols, StringView, arena, name);
}
