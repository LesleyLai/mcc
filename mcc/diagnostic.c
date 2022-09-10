#include "diagnostic.h"
#include "stdio.h"

#include "utils/format.h"

static void write_diagnostic_line(StringBuffer* output, const char* source,
                                  SourceRange range, const char* line_start,
                                  const char* line_end);

void write_diagnostics(StringBuffer* output, const char* file_path,
                       const char* source, ParseError error)
{
  const StringView msg = error.msg;
  const SourceRange range = error.range;

  string_buffer_printf(output, "%s:%u:%u: Error: %.*s\n", file_path,
                       range.begin.line, range.begin.column, (int)msg.size,
                       msg.start);

  bool this_line_in_error = false;
  const char* line_start = source;
  uint32_t line = 1;
  string_buffer_printf(output, "%d |     ", line);

  for (const char* itr = source; *itr != '\0'; ++itr) {
    const size_t offset = (size_t)(itr - source);
    const bool in_error_range =
        offset >= range.begin.offset && offset < range.end.offset;
    this_line_in_error |= in_error_range;

    string_buffer_printf(output, "%c", *itr);

    if (*itr == '\n') {
      // handle windows line ending
      const char* line_end =
          (itr > source && *(itr - 1) == '\r') ? (itr - 1) : itr;

      if (this_line_in_error) {
        write_diagnostic_line(output, source, range, line_start, line_end);
      }
      this_line_in_error = false;
      line_start = itr + 1;
      ++line;
      string_buffer_printf(output, "%d |     ", line);
    }
  }
}

static void write_diagnostic_line(StringBuffer* output, const char* source,
                                  SourceRange range, const char* line_start,
                                  const char* line_end)
{
  string_buffer_printf(output, "  |     ");
  for (const char* itr = line_start; itr < line_end; ++itr) {
    const size_t offset = (size_t)(itr - source);
    if (offset == range.begin.offset) {
      string_buffer_printf(output, "^");
    } else if (offset > range.begin.offset && offset < range.end.offset) {
      string_buffer_printf(output, "~");
    } else {
      string_buffer_printf(output, " ");
    }
  }
  string_buffer_printf(output, "\n");
}
