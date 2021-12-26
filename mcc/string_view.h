#ifndef MCC_STRING_VIEW_H
#define MCC_STRING_VIEW_H

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

#endif // MCC_STRING_VIEW_H
