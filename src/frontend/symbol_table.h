#ifndef MCC_SYMBOL_TABLE_H
#define MCC_SYMBOL_TABLE_H

#include <mcc/arena.h>
#include <mcc/prelude.h>
#include <mcc/str.h>
#include <mcc/type.h>

typedef struct Variable {
  StringView
      name; // name in the source. This is the name used for variable lookup
  StringView rewrote_name; // name after alpha renaming
  uint32_t shadow_counter; // increase each time we have shadowing
  const Type* type;

  bool has_definition; // prevent redefinition
} Variable;

// Represents a block scope
typedef struct Scope Scope;

Scope* new_scope(Scope* parent, Arena* arena);

Variable* lookup_variable(const Scope* scope, StringView name);

// Return nullptr if a variable already exist in the same scope
Variable* add_variable(Scope* scope, StringView name, Arena* arena);

#endif // MCC_SYMBOL_TABLE_H
