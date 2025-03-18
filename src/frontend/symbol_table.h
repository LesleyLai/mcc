#ifndef MCC_SYMBOL_TABLE_H
#define MCC_SYMBOL_TABLE_H

#include <mcc/arena.h>
#include <mcc/hash_table.h>
#include <mcc/prelude.h>
#include <mcc/str.h>
#include <mcc/type.h>

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

/**
 * C has distinction between functions and object identifiers, and they need
 * somewhat different handling.
 */
typedef enum IdentifierTag : uint8_t {
  IDENTIFIER_INVALID,
  IDENTIFIER_FUNCTION,
  IDENTIFIER_OBJECT,
} IdentifierTag;

// Information for all identifiers
typedef struct IdentifierInfo {
  IdentifierTag tag;
  StringView name; // name in the source
  Linkage linkage;
  const Type* type;
} IdentifierInfo;

// Identifier that represent an object
typedef struct ObjectIdentifierInfo {
  IdentifierInfo base;
  StringView rewrote_name; // name after alpha renaming
  uint32_t shadow_counter; // increase each time we have shadowing
  bool has_definition;     // prevent redefinition
} ObjectIdentifierInfo;

// Identifier that represent an object
typedef struct FunctionIdentifierInfo {
  IdentifierInfo base;

  bool has_definition; // prevent redefinition
} FunctionIdentifierInfo;

static inline ObjectIdentifierInfo* as_object_ident(IdentifierInfo* ident)
{
  MCC_ASSERT(ident->tag == IDENTIFIER_OBJECT);
  return (ObjectIdentifierInfo*)ident;
}

static inline FunctionIdentifierInfo* as_function_ident(IdentifierInfo* ident)
{
  MCC_ASSERT(ident->tag == IDENTIFIER_FUNCTION);
  return (FunctionIdentifierInfo*)ident;
}

// Represents a block scope
typedef struct Scope Scope;

// Represents a symbol table in a C translation unit, tracking both scoped and
// global symbols. In C, identifiers with linkage can be referenced across
// multiple scopes while remaining unique within a program. To account for this,
// a pointer to global symbols are stored separately in addition to their
// respective scopes.
typedef struct SymbolTable {
  // Tracks globally visible symbols (those with external or internal linkage)
  HashMap global_symbols;
  // Represents the global scope of the translation unit.
  Scope* global_scope;
} SymbolTable;

Scope* new_scope(Scope* parent, Arena* arena);

IdentifierInfo* lookup_identifier(const Scope* scope, StringView name);

// Add information of the function identifier to the current scope
FunctionIdentifierInfo* add_function_identifer(SymbolTable* symbol_table,
                                               Scope* scope, StringView name,
                                               Linkage linkage, Arena* arena);

// Return nullptr if a variable of the same name already exist in the same scope
// Otherwise we add it to the current scope
IdentifierInfo* add_object_identifier(SymbolTable* symbol_table, Scope* scope,
                                      StringView name, Linkage linkage,
                                      Arena* arena);

#endif // MCC_SYMBOL_TABLE_H
