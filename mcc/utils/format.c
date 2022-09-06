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
  int buffer_size = vsnprintf(NULL, 0, format, args);
  va_end(args);

  if (buffer_size < 0) {
    // TODO: better error handling
    puts("Should not happen!");
    exit(1);
  }

  const size_t buffer_size_u = (size_t)buffer_size;

  const size_t old_size = string_buffer_size(*str);
  const size_t new_size = buffer_size_u + old_size;
  string_buffer_unsafe_resize_for_overwrite(str, new_size);

  const size_t alloc_size = buffer_size_u + 1;
  vsnprintf(string_buffer_data(str) + old_size, alloc_size, format, args_copy);
  va_end(args_copy);
}
