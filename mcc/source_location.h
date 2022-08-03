#ifndef MCC_SOURCE_LOCATION_H
#define MCC_SOURCE_LOCATION_H

#include <stdint.h>

typedef struct SourceLocation {
  uint32_t line;
  uint32_t column;
} SourceLocation;

typedef struct SourceRange {
  SourceLocation first;
  SourceLocation last;
} SourceRange;

#endif // MCC_SOURCE_LOCATION_H
