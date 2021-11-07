#ifndef MCC_STRING_VIEW_H
#define MCC_STRING_VIEW_H

#include <string.h>

typedef struct StringView {
  const char* start;
  size_t length;
} StringView;

inline StringView string_view_create(const char* source)
{
  return (StringView){.start = source, .length = strlen(source)};
}

#endif // MCC_STRING_VIEW_H
