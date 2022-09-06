#ifndef MCC_FORMAT_H
#define MCC_FORMAT_H

#include "str.h"

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
void string_buffer_printf(StringBuffer* str, const char* format, ...);

#endif // MCC_FORMAT_H
