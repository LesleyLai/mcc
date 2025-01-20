#ifndef MCC_FORMAT_H
#define MCC_FORMAT_H

#include "prelude.h"
#include "str.h"

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
void string_buffer_printf(StringBuffer* str, const char* format, ...);

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
StringView
allocate_printf(Arena* arena, const char* format, ...);

#endif // MCC_FORMAT_H
