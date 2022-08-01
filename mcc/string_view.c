#include "string_view.h"

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