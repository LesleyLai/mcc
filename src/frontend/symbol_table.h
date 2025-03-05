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

/**
 * @brief Represents the linkage of a C identifier.
 *
 * Linkage decides the visibility of an identifier and whether multiple
 * declaration refers to the same identifier
 */
typedef enum Linkage {
  // each declaration denotes a unique entity
  LINKAGE_NONE,
  // each declaration denotes the same object or function in the entire program
  LINKAGE_EXTERNAL,
  // each declaration denotes the same object or function  in a single
  // translation unit
  LINKAGE_INTERNAL
} Linkage;

typedef struct IdentifierInfo {
  StringView
      name; // name in the source. This is the name used for variable lookup
  StringView rewrote_name; // name after alpha renaming
  uint32_t shadow_counter; // increase each time we have shadowing

  IdentifierKind kind;
  const Type* type;
  Linkage linkage;

  bool has_definition; // prevent redefinition
} IdentifierInfo;

// Represents a block scope
typedef struct Scope Scope;

Scope* new_scope(Scope* parent, Arena* arena);

IdentifierInfo* lookup_identifier(const Scope* scope, StringView name);

// Return nullptr if a variable of the same name already exist in the same scope
// Otherwise we add it to the current scope
IdentifierInfo* add_identifier(Scope* scope, StringView name,
                               IdentifierKind kind, Linkage linkage,
                               Arena* arena);

#endif // MCC_SYMBOL_TABLE_H
