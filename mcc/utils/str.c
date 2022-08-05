#include "str.h"

StringView string_view_from_c_str(const char* source)
{
  return (StringView){.start = source, .length = strlen(source)};
}

StringView string_view_from_buffer(StringBuffer buffer)
{
  return (StringView){.start = buffer.start, .length = buffer.length};
}

/**
 * @return Length of the result string
 * @note If the size of buffer is not big enough, no modification of buffer will
 * happen. And this function will return 0.
 */
size_t string_view_cat(StringView s1, StringView s2, char* buffer,
                       size_t buffer_size)
{
  const size_t total_length = s1.length + s2.length;
  if (total_length > buffer_size - 1) { return 0; }
  memcpy(buffer, s1.start, s1.length);
  memcpy(buffer + s1.length, s2.start, s2.length);
  buffer[total_length] = '\0';
  return total_length;
}

StringBuffer string_buffer_new(PolyAllocator* allocator)
{
  return (StringBuffer){
      .start = NULL, .length = 0, .capacity = 0, .allocator = allocator};
}