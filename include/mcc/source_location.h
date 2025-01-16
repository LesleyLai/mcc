#ifndef MCC_SOURCE_LOCATION_H
#define MCC_SOURCE_LOCATION_H

#include "str.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct SourceRange {
  uint32_t begin; // offset into the start index in the source code
  uint32_t
      end; // offset into the position after the last character in the range
} SourceRange;

/// @brief A table used to compute line/column numbers
typedef struct LineNumTable {
  uint32_t line_count;
  const uint32_t* line_starts;
} LineNumTable;

typedef struct LineColumn {
  uint32_t line;
  uint32_t column;
} LineColumn;

/**
 * Get the table for calculate line numbers of a file. If the table is already
 * initialized, just return it. Otherwise create the table.
 */
const LineNumTable* get_line_num_table(const char* filename, StringView source,
                                       Arena* permanent_arena,
                                       Arena scratch_arena);

LineColumn calculate_line_and_column(const LineNumTable* table,
                                     uint32_t offset);

#endif // MCC_SOURCE_LOCATION_H
