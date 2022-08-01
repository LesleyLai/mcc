#ifndef MCC_STRING_VIEW_H
#define MCC_STRING_VIEW_H

#include <stddef.h>
#include <string.h>

typedef struct StringView {
  const char* start;
  size_t length;
} StringView;

static inline StringView string_view_create(const char* source)
{
  StringView sv;
  sv.start = source;
  sv.length = strlen(source);
  return sv;
}

/**
 * Concatenate content of two StringView and store the result in buffer
 * @return Length of the result string
 * @note If the size of buffer is not big enough, no modification of buffer will
 * happen. And this function will return 0.
 */
size_t string_view_cat(StringView s1, StringView s2, char* buffer,
                       size_t buffer_size);

#endif // MCC_STRING_VIEW_H
