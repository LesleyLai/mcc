#include "str.h"
#include <string.h>

StringView string_view_from_c_str(const char* source)
{
  return (StringView){.start = source, .length = strlen(source)};
}

StringView string_view_from_buffer(StringBuffer buffer)
{
  return (StringView){.start = buffer.start, .length = buffer.length};
}

bool string_view_eq(StringView lhs, StringView rhs)
{
  if (lhs.length != rhs.length) return false;
  for (size_t i = 0; i < lhs.length; ++i) {
    if (lhs.start[i] != rhs.start[i]) { return false; }
  }
  return true;
}

#define STRING_BUFFER_INITIAL_ALLOC 16

StringBuffer string_buffer_new(PolyAllocator* allocator)
{
  StringBuffer buffer =
      (StringBuffer){.start = poly_aligned_alloc(allocator, alignof(char),
                                                 STRING_BUFFER_INITIAL_ALLOC),
                     .length = 0,
                     .capacity = STRING_BUFFER_INITIAL_ALLOC - 1,
                     .allocator = allocator};

  buffer.start[0] = '\0';
  return buffer;
}

StringBuffer string_buffer_from_c_str(const char* source,
                                      PolyAllocator* allocator)
{
  return string_buffer_from_view(string_view_from_c_str(source), allocator);
}

StringBuffer string_buffer_from_view(StringView source,
                                     PolyAllocator* allocator)
{
  StringBuffer buffer = (StringBuffer){
      .start = poly_aligned_alloc(allocator, alignof(char), source.length),
      .length = source.length,
      .capacity = source.length,
      .allocator = allocator};
  memcpy(buffer.start, source.start, source.length);
  buffer.start[source.length] = '\0';
  return buffer;
}

static void _string_buffer_grow(StringBuffer* self, size_t new_capacity)
{
  // No need to free old memory since we only use arena allocation

  assert(new_capacity > self->capacity);
  self->capacity = new_capacity;
  char* new_start =
      poly_aligned_alloc(self->allocator, alignof(char), new_capacity + 1);
  memcpy(new_start, self->start, self->length);
  self->start = new_start;
  // self->start[self->length] = '\0';
}

void string_buffer_push(StringBuffer* self, char c)
{
  if (self->length >= self->capacity) {
    size_t new_capacity = self->capacity < 8 ? 16 : self->capacity * 2;
    _string_buffer_grow(self, new_capacity);
  }
  self->start[self->length++] = c;
  self->start[self->length] = '\0';
}

void string_buffer_append(StringBuffer* self, StringView rhs)
{
  size_t new_length = self->length + rhs.length;
  if (new_length > self->capacity) { _string_buffer_grow(self, new_length); }
  memcpy(self->start + self->length, rhs.start, rhs.length);
  self->length = new_length;
  self->start[self->length] = '\0';
}