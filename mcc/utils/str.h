#ifndef MCC_STR_H
#define MCC_STR_H

#include <stddef.h>
#include <string.h>

#include "allocators.h"

typedef struct StringView {
  const char* start;
  size_t length;
} StringView;

typedef struct StringBuffer {
  const char* start;
  size_t length;
  size_t capacity;
  PolyAllocator* allocator;
} StringBuffer;

// Circumvent C/C++ incompatibility of compound literal
#ifdef __cplusplus
#define MCC_COMPOUND_LITERAL(T) T
#else
#define MCC_COMPOUND_LITERAL(T) (T)
#endif

StringView string_view_from_c_str(const char* source);
StringView string_view_from_buffer(StringBuffer buffer);

StringBuffer string_buffer_new(PolyAllocator* allocator);

/**
 * @brief Concatenate content of two StringView and store the result in buffer
 */
size_t string_view_cat(StringView s1, StringView s2, char* buffer,
                       size_t buffer_size);

#endif // MCC_STR_H
