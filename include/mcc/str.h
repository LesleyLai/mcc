#ifndef MCC_STR_H
#define MCC_STR_H

#include "arena.h"
#include "prelude.h"

// A view of a slice of string
typedef struct StringView {
  const char* start;
  size_t size;
} StringView;

// A dynamic array of characters used to build strings
// Like C++'s std::string, the underlying buffer is always null-terminated
typedef struct StringBuffer {
  size_t size_;
  size_t capacity_;
  char* data_;
  Arena* allocator;
} StringBuffer;

StringView str(const char* source); // Create a string view from c string
StringView str_from_buffer(const StringBuffer* buffer);
bool str_eq(StringView lhs, StringView rhs);
bool str_start_with(StringView s, StringView start);

StringBuffer string_buffer_new(Arena* allocator);
StringBuffer string_buffer_from_c_str(const char* source, Arena* allocator);
StringBuffer string_buffer_from_view(StringView source, Arena* allocator);
size_t string_buffer_size(StringBuffer self);
size_t string_buffer_capacity(StringBuffer self);
const char* string_buffer_c_str(const StringBuffer* self);
char* string_buffer_data(StringBuffer* self);

void string_buffer_push(StringBuffer* self, char c);
void string_buffer_append(StringBuffer* self, StringView rhs);

// Don't use it unless you know what you are doing!!!
void string_buffer_unsafe_resize_for_overwrite(StringBuffer* self,
                                               size_t count);

#endif // MCC_STR_H
