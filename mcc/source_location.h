#ifndef MCC_SOURCE_LOCATION_H
#define MCC_SOURCE_LOCATION_H

#include <stdint.h>
#include <stdlib.h>

typedef struct SourceLocation {
  uint32_t line;
  uint32_t column;
  size_t offset; // A flat offset from the start of the source file
} SourceLocation;

typedef struct SourceRange {
  SourceLocation begin;
  SourceLocation end;
} SourceRange;

#endif // MCC_SOURCE_LOCATION_H
