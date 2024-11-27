#include "str.h"

#include <string.h>

#include <stdlib.h>

static bool _string_buffer_is_large(StringBuffer self)
{
  // Is the lowest bit of the first byte 1?
  return (self.data_.small_.size_with_bit_mark_ & 1) != 0;
}

static void _string_buffer_mark_large(StringBuffer* self)
{
  // Sets the lowest bit of the first byte to 1
  self->data_.large_.size_with_bit_mark_ |= 1;
}

StringView string_view_from_c_str(const char* source)
{
  return (StringView){.start = source, .size = strlen(source)};
}

bool string_view_eq(StringView lhs, StringView rhs)
{
  if (lhs.size != rhs.size) return false;
  for (size_t i = 0; i < lhs.size; ++i) {
    if (lhs.start[i] != rhs.start[i]) { return false; }
  }
  return true;
}

StringBuffer string_buffer_new(Arena* allocator)
{
  return (StringBuffer){.data_ = {.small_ = {.size_with_bit_mark_ = 0}},
                        .allocator = allocator};
}

StringBuffer string_buffer_from_c_str(const char* source, Arena* allocator)
{
  return string_buffer_from_view(string_view_from_c_str(source), allocator);
}

StringBuffer string_buffer_from_view(StringView source, Arena* allocator)
{
  StringBuffer buffer = {.allocator = allocator};
  char* data_ptr = NULL;

  if (source.size <= small_string_capacity) {
    buffer.data_.small_ = (struct StringSmallBuffer_){
        .size_with_bit_mark_ = (unsigned char)(source.size << 1)};
    data_ptr = buffer.data_.small_.data_;
  } else {
    buffer.data_.large_ = (struct StringLargeBuffer_){
        .size_with_bit_mark_ = source.size << 1,
        .capacity_ = source.size,
        .data_ = arena_aligned_alloc(allocator, alignof(char), source.size + 1),
    };
    _string_buffer_mark_large(&buffer);
    data_ptr = buffer.data_.large_.data_;
  }

  memcpy(data_ptr, source.start, source.size);
  data_ptr[source.size] = '\0';

  return buffer;
}

static size_t _string_buffer_size_small(StringBuffer self)
{
  return self.data_.small_.size_with_bit_mark_ >> 1;
}

static size_t _string_buffer_size_large(StringBuffer self)
{
  return self.data_.large_.size_with_bit_mark_ >> 1;
}

StringView string_view_from_buffer(const StringBuffer* buffer)
{
  return (StringView){.start = string_buffer_c_str(buffer),
                      .size = string_buffer_size(*buffer)};
}

const char* string_buffer_c_str(const StringBuffer* self)
{
  return _string_buffer_is_large(*self) ? self->data_.large_.data_
                                        : self->data_.small_.data_;
}

char* string_buffer_data(StringBuffer* self)
{
  return _string_buffer_is_large(*self) ? self->data_.large_.data_
                                        : self->data_.small_.data_;
}

size_t string_buffer_size(StringBuffer self)
{
  return _string_buffer_is_large(self) ? _string_buffer_size_large(self)
                                       : _string_buffer_size_small(self);
}

size_t string_buffer_capacity(StringBuffer self)
{
  return _string_buffer_is_large(self) ? (self.data_.large_.capacity_)
                                       : small_string_capacity;
}

static void _string_buffer_grow_large(StringBuffer* self, size_t new_capacity)
{
  // No need to free old memory since we only use arena allocation
  assert(new_capacity > self->data_.large_.capacity_ ||
         new_capacity > small_string_capacity);
  self->data_.large_.capacity_ = new_capacity;
  self->data_.large_.data_ =
      arena_aligned_grow(self->allocator, self->data_.large_.data_,
                         alignof(char), new_capacity + 1);
}

static void _string_buffer_push_small(StringBuffer* self, char c)
{
  const size_t old_size = _string_buffer_size_small(*self);
  if (old_size < small_string_capacity) {
    // If still small
    self->data_.small_.data_[old_size] = c;
    self->data_.small_.data_[old_size + 1] = '\0';
    self->data_.small_.size_with_bit_mark_ += 2;
  } else {
    // small to large
    size_t new_capacity = small_string_capacity * 2;
    char* new_start =
        arena_aligned_alloc(self->allocator, alignof(char), new_capacity + 1);
    memcpy(new_start, self->data_.small_.data_, small_string_capacity);
    new_start[small_string_capacity] = c;
    new_start[small_string_capacity + 1] = '\0';

    self->data_.large_ = (struct StringLargeBuffer_){
        .size_with_bit_mark_ = (small_string_capacity + 1) << 1,
        .capacity_ = new_capacity,
        .data_ = new_start,
    };
    _string_buffer_mark_large(self);
  }
}

static void _string_buffer_push_large(StringBuffer* self, char c)
{
  const size_t old_size = _string_buffer_size_large(*self);
  const size_t old_capacity = self->data_.large_.capacity_;
  if (old_size > old_capacity) {
    _string_buffer_grow_large(self, old_capacity * 2);
  }

  self->data_.large_.data_[old_size] = c;
  self->data_.large_.data_[old_size + 1] = '\0';
  self->data_.large_.size_with_bit_mark_ += 2;
  assert(_string_buffer_is_large(*self));
}

void string_buffer_push(StringBuffer* self, char c)
{
  if (_string_buffer_is_large(*self)) {
    _string_buffer_push_large(self, c);
  } else {
    _string_buffer_push_small(self, c);
  }
}

static void _string_buffer_append_small(StringBuffer* self, StringView rhs)
{
  const size_t old_size = _string_buffer_size_small(*self);
  const size_t new_size = old_size + rhs.size;
  if (new_size <= small_string_capacity) {
    self->data_.small_.size_with_bit_mark_ = (unsigned char)(new_size << 1);
    memcpy(self->data_.small_.data_ + old_size, rhs.start, rhs.size);
    self->data_.small_.data_[new_size] = '\0';
  } else {
    char* new_data =
        arena_aligned_alloc(self->allocator, alignof(char), new_size + 1);
    memcpy(new_data, self->data_.small_.data_,
           _string_buffer_size_small(*self));
    memcpy(new_data + old_size, rhs.start, rhs.size);

    self->data_.large_ = (struct StringLargeBuffer_){
        .size_with_bit_mark_ = (unsigned char)(new_size << 1),
        .capacity_ = new_size,
        .data_ = new_data};
    _string_buffer_mark_large(self);
    new_data[new_size] = '\0';
  }
}

static void _string_buffer_append_large(StringBuffer* self, StringView rhs)
{
  const size_t old_size = _string_buffer_size_large(*self);
  const size_t new_size = old_size + rhs.size;
  const size_t old_capacity = self->data_.large_.capacity_;
  if (new_size > old_capacity) { _string_buffer_grow_large(self, new_size); }
  memcpy(self->data_.large_.data_ + old_size, rhs.start, rhs.size);
  self->data_.large_.data_[new_size] = '\0';
  self->data_.large_.size_with_bit_mark_ = new_size << 1;
  _string_buffer_mark_large(self);
}

void string_buffer_append(StringBuffer* self, StringView rhs)
{
  if (_string_buffer_is_large(*self)) {
    _string_buffer_append_large(self, rhs);
  } else {
    _string_buffer_append_small(self, rhs);
  }
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
  const size_t old_size = string_buffer_size(*self);
  const size_t old_capacity = string_buffer_capacity(*self);
  const size_t new_size = count;

  if (new_size <= old_capacity) {
    if (_string_buffer_is_large(*self)) {
      self->data_.large_.size_with_bit_mark_ = new_size << 1;
      _string_buffer_mark_large(self);
    } else {
      self->data_.small_.size_with_bit_mark_ = (unsigned char)(new_size << 1);
    }

    return;
  }

  if (_string_buffer_is_large(*self)) {
    _string_buffer_grow_large(self, new_size);
    self->data_.large_.size_with_bit_mark_ = new_size << 1;
    _string_buffer_mark_large(self);
  } else {
    if (new_size <= small_string_capacity) { // small to small
      self->data_.small_.size_with_bit_mark_ = (unsigned char)(new_size << 1);
    } else { // small to large
      char* new_data =
          arena_aligned_alloc(self->allocator, alignof(char), new_size + 1);
      memcpy(new_data, self->data_.small_.data_, old_size);

      self->data_.large_ = (struct StringLargeBuffer_){
          .size_with_bit_mark_ = new_size << 1,
          .capacity_ = new_size,
          .data_ = new_data,
      };
      _string_buffer_mark_large(self);
    }
  }
}
