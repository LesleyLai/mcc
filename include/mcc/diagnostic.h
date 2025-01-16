#ifndef MCC_DIAGNOSTIC_H
#define MCC_DIAGNOSTIC_H

#include "source_location.h"
#include "str.h"

typedef struct Error {
  StringView msg;
  SourceRange range;
} Error;

typedef struct ErrorsView {
  size_t length;
  Error* data;
} ErrorsView;

typedef struct DiagnosticsContext {
  const char* filename;
  StringView source;
  const LineNumTable* line_num_table;
} DiagnosticsContext;

DiagnosticsContext create_diagnostic_context(const char* filename,
                                             StringView source,
                                             Arena* permanent_arena,
                                             Arena scratch_arena);

void print_parse_diagnostics(ErrorsView errors,
                             const DiagnosticsContext* context);

void write_diagnostics(StringBuffer* output, const Error* error,
                       const DiagnosticsContext* context);

#endif // MCC_DIAGNOSTIC_H
