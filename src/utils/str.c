#include <mcc/str.h>

#include <stdlib.h>
#include <string.h>

static_assert(sizeof(StringView) == 2 * sizeof(void*),
              "Size of a StringView should be 2 pointer size");

static_assert(sizeof(StringBuffer) == 4 * sizeof(void*),
              "Size of a StringView should be 4 pointer size");

StringView str(const char* source)
{
  return (StringView){.start = source, .size = strlen(source)};
}

bool str_eq(StringView lhs, StringView rhs)
{
  if (lhs.size != rhs.size) return false;
  return strncmp(lhs.start, rhs.start, lhs.size) == 0;
}

bool str_start_with(StringView s, StringView start)
{
  if (s.size < start.size) { return false; }
  return strncmp(s.start, start.start, start.size) == 0;
}

StringBuffer string_buffer_new(Arena* allocator)
{
  return (StringBuffer){.allocator = allocator};
}

StringBuffer string_buffer_from_c_str(const char* source, Arena* allocator)
{
  return string_buffer_from_view(str(source), allocator);
}

StringBuffer string_buffer_from_view(StringView source, Arena* allocator)
{
  StringBuffer buffer = (StringBuffer){
      .allocator = allocator,
      .capacity_ = source.size,
      .size_ = source.size,
      .data_ = ARENA_ALLOC_ARRAY(allocator, char, source.size + 1),
  };
  memcpy(buffer.data_, source.start, source.size);
  buffer.data_[source.size] = '\0';
  return buffer;
}

StringView str_from_buffer(const StringBuffer* buffer)
{
  return (StringView){.start = string_buffer_c_str(buffer),
                      .size = string_buffer_size(*buffer)};
}

const char* string_buffer_c_str(const StringBuffer* self)
{
  return self->data_;
}

char* string_buffer_data(StringBuffer* self)
{
  return self->data_;
}

size_t string_buffer_size(StringBuffer self)
{
  return self.size_;
}

size_t string_buffer_capacity(StringBuffer self)
{
  return self.capacity_;
}

static void string_buffer_grow(StringBuffer* self, size_t new_capacity)
{
  // No need to free old memory since we only use arena allocation
  MCC_ASSERT_MSG(new_capacity > self->capacity_,
                 "Only call string_buffer_grow when the capacity of self is "
                 "not big enough");
  self->data_ =
      arena_aligned_realloc(self->allocator, self->data_, alignof(char),
                            self->capacity_ + 1, new_capacity + 1);
  self->capacity_ = new_capacity;
}

void string_buffer_push(StringBuffer* self, char c)
{
  const size_t old_size = self->size_;
  const size_t old_capacity = self->capacity_;
  if (old_size >= old_capacity) {
    const size_t double_capacity = old_capacity * 2;
    string_buffer_grow(self, 16 > double_capacity ? 16 : double_capacity);
  }

  self->data_[old_size] = c;
  self->data_[old_size + 1] = '\0';
  self->size_ += 1;
}

void string_buffer_append(StringBuffer* self, StringView rhs)
{
  const size_t old_size = self->size_;
  const size_t new_size = old_size + rhs.size;
  const size_t old_capacity = self->capacity_;
  if (new_size > old_capacity) { string_buffer_grow(self, new_size); }
  memcpy(self->data_ + old_size, rhs.start, rhs.size);
  self->data_[new_size] = '\0';
  self->size_ = new_size;
}

/**
 * @brief Resize the string buffer to contain `size` characters but does
not write to the underlying buffer
 * @warning Very dangerous since it break invariant of the string buffer. It
 * should *always* be followed by an immediate write to the underlying data of
 * the underlying buffer.
 * @warning There are no guarantee that the result String Buffer contains
 * null-terminated data.
 *
 * If count <= size()
 */
void string_buffer_unsafe_resize_for_overwrite(StringBuffer* self, size_t count)
{
  const size_t old_capacity = string_buffer_capacity(*self);
  const size_t new_size = count;

  if (new_size <= old_capacity) {
    self->size_ = new_size;
    return;
  }

  string_buffer_grow(self, new_size);
  self->size_ = new_size;
}
