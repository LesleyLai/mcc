#include "symbol_table.h"

#include <mcc/dynarray.h>
#include <mcc/format.h>

// TODO: use hash table
struct Scope {
  uint32_t length;
  uint32_t capacity;
  Variable* data;
  struct Scope* parent;
};

Scope* new_scope(Scope* parent, Arena* arena)
{
  struct Scope* map = ARENA_ALLOC_OBJECT(arena, Scope);
  *map = (struct Scope){
      .parent = parent,
  };
  return map;
}

Variable* lookup_variable(const Scope* scope, StringView name)
{
  for (uint32_t i = 0; i < scope->length; ++i) {
    if (str_eq(name, scope->data[i].name)) { return &scope->data[i]; }
  }

  if (scope->parent == nullptr) return nullptr;

  return lookup_variable(scope->parent, name);
}

Variable* add_variable(StringView name, Scope* scope, Arena* arena)
{
  // check variable in current scope
  for (uint32_t i = 0; i < scope->length; ++i) {
    if (str_eq(name, scope->data[i].name)) { return nullptr; }
  }

  // lookup variable in parent scopes
  const Variable* parent_variable = nullptr;
  if (scope->parent) { parent_variable = lookup_variable(scope->parent, name); }

  Variable variable;
  if (parent_variable == nullptr) {
    variable = (Variable){
        .name = name,
        .rewrote_name = name,
        .shadow_counter = 0,
    };
  } else {
    uint32_t shadow_counter = parent_variable->shadow_counter + 1;
    variable = (Variable){
        .name = name,
        .rewrote_name = allocate_printf(arena, "%.*s.%i", (int)name.size,
                                        name.start, shadow_counter),
        .shadow_counter = shadow_counter + 1,
    };
  }

  DYNARRAY_PUSH_BACK(scope, Variable, arena, variable);
  return &scope->data[scope->length - 1];
}
