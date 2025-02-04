#ifndef MCC_SYMBOL_TABLE_H
#define MCC_SYMBOL_TABLE_H

#include <mcc/arena.h>
#include <mcc/hash_table.h>
#include <mcc/prelude.h>
#include <mcc/str.h>
#include <mcc/type.h>

typedef enum IdentifierKind {
  IDENT_INVALID = 0,
  IDENT_OBJECT,
  IDENT_FUNCTION,
} IdentifierKind;

typedef struct Identifier {
  StringView
      name; // name in the source. This is the name used for variable lookup
  StringView rewrote_name; // name after alpha renaming
  uint32_t shadow_counter; // increase each time we have shadowing

  IdentifierKind kind;
  const Type* type;

  bool has_definition; // prevent redefinition
} Identifier;

// Represents a block scope
typedef struct Scope Scope;

Scope* new_scope(Scope* parent, Arena* arena);

Identifier* lookup_identifier(const Scope* scope, StringView name);

// Return nullptr if a variable of the same name already exist in the same scope
// Otherwise we add it to the current scope
Identifier* add_identifier(Scope* scope, StringView name, IdentifierKind kind,
                           Arena* arena);

// Besides stored in the scopes, the identifiers of functions in a translation
// unit are also refered to in a separate hash table. This is useful because
// each named only map to one instance of a function no matter the scope
extern HashMap functions;

#endif // MCC_SYMBOL_TABLE_H
