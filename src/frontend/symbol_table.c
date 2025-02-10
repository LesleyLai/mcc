#include "symbol_table.h"

#include <mcc/format.h>
#include <mcc/hash_table.h>

struct Scope {
  HashMap identifiers;
  struct Scope* parent;
};

Scope* new_scope(Scope* parent, Arena* arena)
{
  struct Scope* map = ARENA_ALLOC_OBJECT(arena, Scope);
  *map = (struct Scope){
      .identifiers = (HashMap){},
      .parent = parent,
  };
  return map;
}

Identifier* lookup_identifier(const Scope* scope, StringView name)
{
  Identifier* identifier = hashmap_lookup(&scope->identifiers, name);
  if (identifier != nullptr) return identifier;
  if (scope->parent == nullptr) return nullptr;

  return lookup_identifier(scope->parent, name);
}

Identifier* add_identifier(Scope* scope, StringView name, IdentifierKind kind,
                           Arena* arena)
{
  // check variable in current scope
  if (hashmap_lookup(&scope->identifiers, name) != nullptr) { return nullptr; }

  // lookup variable in parent scopes
  const Identifier* parent_variable = nullptr;
  if (scope->parent) {
    parent_variable = lookup_identifier(scope->parent, name);
  }

  Identifier* variable = ARENA_ALLOC_OBJECT(arena, Identifier);
  if (parent_variable == nullptr) {
    *variable = (Identifier){
        .name = name,
        .rewrote_name = name,
        .kind = kind,
        .shadow_counter = 0,
    };
  } else {
    uint32_t shadow_counter = parent_variable->shadow_counter + 1;
    *variable = (Identifier){
        .name = name,
        .rewrote_name = allocate_printf(arena, "%.*s.%i", (int)name.size,
                                        name.start, shadow_counter),
        .kind = kind,
        .shadow_counter = shadow_counter + 1,
    };
  }

  hashmap_insert(&scope->identifiers, name, variable, arena);
  return variable;
}
