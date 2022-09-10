#ifndef MCC_DIAGNOSTIC_H
#define MCC_DIAGNOSTIC_H

#include "source_location.h"
#include "utils/str.h"

typedef struct ParseError {
  StringView msg;
  SourceRange range;
} ParseError;

void write_diagnostics(StringBuffer* output, const char* file_path,
                       const char* source, ParseError error);

#endif // MCC_DIAGNOSTIC_H
