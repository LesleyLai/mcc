#ifndef MCC_STR_H
#define MCC_STR_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "allocators.h"

typedef struct StringView {
  const char* start;
  size_t size;
} StringView;

enum { small_string_capacity = 3 * sizeof(void*) - 2 };
struct StringSmallBuffer_ {
  unsigned char size_with_bit_mark_;
  char data_[small_string_capacity + 1];
};

struct StringLargeBuffer_ {
  size_t size_with_bit_mark_;
  size_t capacity_;
  char* data_;
};

typedef struct StringBuffer {
  union {
    struct StringSmallBuffer_ small_;
    struct StringLargeBuffer_ large_;
  } data_;

  PolyAllocator* allocator;
} StringBuffer;

static_assert(sizeof(StringView) == 2 * sizeof(void*),
              "Size of a StringView should be 2 pointer size");

static_assert(sizeof(StringBuffer) == 4 * sizeof(void*),
              "Size of a StringView should be 4 pointer size");

StringView string_view_from_c_str(const char* source);
StringView string_view_from_buffer(const StringBuffer* buffer);
bool string_view_eq(StringView lhs, StringView rhs);

StringBuffer string_buffer_new(PolyAllocator* allocator);
StringBuffer string_buffer_from_c_str(const char* source,
                                      PolyAllocator* allocator);
StringBuffer string_buffer_from_view(StringView source,
                                     PolyAllocator* allocator);
size_t string_buffer_size(StringBuffer self);
size_t string_buffer_capacity(StringBuffer self);
const char* string_buffer_const_data(const StringBuffer* self);
char* string_buffer_data(StringBuffer* self);

void string_buffer_push(StringBuffer* self, char c);
void string_buffer_append(StringBuffer* self, StringView rhs);

#endif // MCC_STR_H
