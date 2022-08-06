#ifndef MCC_STR_H
#define MCC_STR_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "allocators.h"

// Circumvent C/C++ incompatibility of compound literal
#ifdef __cplusplus
#define MCC_COMPOUND_LITERAL(T) T
#else
#define MCC_COMPOUND_LITERAL(T) (T)
#endif

typedef struct StringView {
  const char* start;
  size_t length;
} StringView;

typedef struct StringBuffer {
  char* start;
  size_t length;
  size_t capacity;
  PolyAllocator* allocator;
} StringBuffer;

static_assert(sizeof(StringView) == 2 * sizeof(void*),
              "Size of a StringView should be 2 pointer size");
static_assert(sizeof(StringBuffer) == 4 * sizeof(void*),
              "Size of a StringView should be 4 pointer size");

StringView string_view_from_c_str(const char* source);
StringView string_view_from_buffer(StringBuffer buffer);
bool string_view_eq(StringView lhs, StringView rhs);

StringBuffer string_buffer_new(PolyAllocator* allocator);
StringBuffer string_buffer_from_c_str(const char* source,
                                      PolyAllocator* allocator);
StringBuffer string_buffer_from_view(StringView source,
                                     PolyAllocator* allocator);
void string_buffer_push(StringBuffer* self, char c);
void string_buffer_append(StringBuffer* self, StringView rhs);

#endif // MCC_STR_H
