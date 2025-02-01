#ifndef MCC_TYPE_H
#define MCC_TYPE_H

#include <stddef.h>
#include <stdint.h>

#include "arena.h"
#include "str.h"

typedef enum TypeTag : uint8_t {
  TYPE_VOID,
  TYPE_INTEGER,
  TYPE_FUNCTION,
} TypeTag;

typedef struct Type {
  uint32_t size;
  uint32_t alignment;
  alignas(max_align_t) TypeTag tag;
} Type;

typedef struct IntegerType {
  Type base;
  bool is_unsigned;
  const char* name;
} IntegerType;

typedef struct FunctionType {
  Type base;
  const Type* return_type;
  uint32_t param_count;
} FunctionType;

extern const Type* typ_void;
extern const Type* typ_int;

const Type* func_type(const Type* return_type, uint32_t param_count,
                      Arena* arena);

void format_type_to(StringBuffer* buffer, const Type* typ);

#endif // MCC_TYPE_H
