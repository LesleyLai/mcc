#include "format.h"

#include <stdarg.h>
#include <stdio.h>

#include <stdlib.h>

void string_buffer_printf(StringBuffer* str, const char* restrict format, ...)
{
  va_list args;
  va_start(args, format);
  va_list args_copy;
  va_copy(args_copy, args);
  const int buffer_size = vsnprintf(NULL, 0, format, args);
  va_end(args);

  MCC_ASSERT_MSG(buffer_size >= 0, "Buffer size must be greater than 0");

  const size_t buffer_size_u = (size_t)buffer_size;

  const size_t old_size = string_buffer_size(*str);
  const size_t new_size = buffer_size_u + old_size;
  string_buffer_unsafe_resize_for_overwrite(str, new_size);

  const size_t alloc_size = buffer_size_u + 1;
  const int result = vsnprintf(string_buffer_data(str) + old_size, alloc_size,
                               format, args_copy);
  MCC_ASSERT_MSG(result == buffer_size, "Successfully writing result");

  va_end(args_copy);
}

char* allocate_printf(Arena* arena, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  va_list args_copy;
  va_copy(args_copy, args);
  int buffer_size = vsnprintf(NULL, 0, format, args);
  va_end(args);

  MCC_ASSERT_MSG(buffer_size > 0, "Buffer size must be greater than 0");

  size_t alloc_size = (size_t)buffer_size + 1;

  char* buffer = ARENA_ALLOC_ARRAY(arena, char, alloc_size);
  const int result = vsnprintf(buffer, alloc_size, format, args_copy);
  MCC_ASSERT_MSG(result == buffer_size, "Successfully writing result");

  return buffer;
}
