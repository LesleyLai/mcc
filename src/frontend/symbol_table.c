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

IdentifierInfo* lookup_identifier(const Scope* scope, StringView name)
{
  IdentifierInfo* identifier = hashmap_lookup(&scope->identifiers, name);
  if (identifier != nullptr) return identifier;
  if (scope->parent == nullptr) return nullptr;

  return lookup_identifier(scope->parent, name);
}

static IdentifierInfo* add_identifier_to_scope(Scope* scope, StringView name,
                                               Linkage linkage, Arena* arena)
{
  // lookup variable in parent scopes
  IdentifierInfo* parent_variable = nullptr;
  if (scope->parent) {
    parent_variable = lookup_identifier(scope->parent, name);
  }

  ObjectIdentifierInfo* variable =
      ARENA_ALLOC_OBJECT(arena, ObjectIdentifierInfo);
  if (parent_variable == nullptr) {
    *variable = (ObjectIdentifierInfo){
        .base =
            {
                .tag = IDENTIFIER_OBJECT,
                .name = name,
                .linkage = linkage,
            },
        .rewrote_name = name,
        .shadow_counter = 0,
    };
  } else {
    const uint32_t shadow_counter =
        as_object_ident(parent_variable)->shadow_counter + 1;
    *variable = (ObjectIdentifierInfo){
        .base =
            {
                .tag = IDENTIFIER_OBJECT,
                .name = name,
                .linkage = linkage,
            },
        .rewrote_name = allocate_printf(arena, "%.*s.%i", (int)name.size,
                                        name.start, shadow_counter),
        .shadow_counter = shadow_counter,
    };
  }

  const bool insert_succeed =
      hashmap_try_insert(&scope->identifiers, name, variable, arena);
  MCC_ASSERT(insert_succeed);

  return (IdentifierInfo*)variable;
}

FunctionIdentifierInfo* add_function_identifer(SymbolTable* symbol_table,
                                               Scope* scope, StringView name,
                                               Linkage linkage, Arena* arena)
{
  MCC_ASSERT(linkage != LINKAGE_NONE);

  // TODO: proper types
  FunctionIdentifierInfo* info =
      ARENA_ALLOC_OBJECT(arena, FunctionIdentifierInfo);
  *info = (FunctionIdentifierInfo){.base = {
                                       .tag = IDENTIFIER_FUNCTION,
                                       .name = name,
                                       .linkage = linkage,
                                   }};

  hashmap_try_insert(&scope->identifiers, name, info, arena);
  hashmap_try_insert(&symbol_table->global_symbols, name, info, arena);

  return info;
}

IdentifierInfo* add_object_identifier(SymbolTable* symbol_table, Scope* scope,
                                      StringView name, Linkage linkage,
                                      Arena* arena)
{
  // check identifier in current scope
  if (hashmap_lookup(&scope->identifiers, name) != nullptr) { return nullptr; }

  IdentifierInfo* ident_info =
      add_identifier_to_scope(scope, name, linkage, arena);

  if (linkage != LINKAGE_NONE) {
    hashmap_try_insert(&symbol_table->global_symbols, name, ident_info, arena);
  }

  return ident_info;
}
