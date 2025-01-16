#include <mcc/diagnostic.h>
#include <mcc/frontend.h>

#include <mcc/format.h>
#include <stdio.h>

DiagnosticsContext create_diagnostic_context(const char* filename,
                                             StringView source,
                                             Arena* permanent_arena,
                                             Arena scratch_arena)
{
  const LineNumTable* line_num_table =
      get_line_num_table(filename, source, permanent_arena, scratch_arena);
  return (DiagnosticsContext){
      .filename = filename, .source = source, .line_num_table = line_num_table};
}

static void write_diagnostic_line(StringBuffer* output, SourceRange error_range,
                                  uint32_t line_begin, uint32_t line_end);

void write_diagnostics(StringBuffer* output, const Error* error,
                       const DiagnosticsContext* context)
{
  const char* file_path = context->filename;
  const StringView source = context->source;

  const StringView msg = error->msg;
  const SourceRange error_range = error->range;

  const LineColumn line_column =
      calculate_line_and_column(context->line_num_table, error_range.begin);
  string_buffer_printf(output, "%s:%u:%u: Error: %.*s\n", file_path,
                       line_column.line, line_column.column, (int)msg.size,
                       msg.start);

  bool this_line_in_error = false;
  uint32_t line_begin = 0;
  uint32_t line = 1;
  string_buffer_printf(output, "%d |     ", line);

  for (uint32_t i = 0; i < source.size; ++i) {
    const bool in_error_range = i >= error_range.begin && i < error_range.end;
    this_line_in_error |= in_error_range;

    const char c = source.start[i];

    string_buffer_printf(output, "%c", c);

    if (c == '\n') {
      // handle windows line ending
      const uint32_t line_end =
          (i != 0 && source.start[i - 1] == '\r') ? (i - 1) : i;

      if (this_line_in_error) {
        write_diagnostic_line(output, error_range, line_begin, line_end);
      }
      this_line_in_error = false;
      line_begin = i + 1;
      ++line;
      string_buffer_printf(output, "%d |     ", line);
    }
  }
  string_buffer_printf(output, "\n");
}

static void write_diagnostic_line(StringBuffer* output, SourceRange error_range,
                                  uint32_t line_begin, uint32_t line_end)
{
  string_buffer_printf(output, "  |     ");
  for (uint32_t i = line_begin; i < line_end; ++i) {
    if (i == error_range.begin) {
      string_buffer_printf(output, "^");
    } else if (i > error_range.begin && i < error_range.end) {
      string_buffer_printf(output, "~");
    } else {
      string_buffer_printf(output, " ");
    }
  }
  string_buffer_printf(output, "\n");
}

void print_parse_diagnostics(ErrorsView errors,
                             const DiagnosticsContext* context)
{
  enum { diagnostics_arena_size = 40000 }; // 40 Mb
  uint8_t diagnostics_buffer[diagnostics_arena_size];
  Arena diagnostics_arena =
      arena_init(diagnostics_buffer, diagnostics_arena_size);

  for (size_t i = 0; i < errors.length; ++i) {
    StringBuffer output = string_buffer_new(&diagnostics_arena);
    write_diagnostics(&output, &errors.data[i], context);
    StringView output_view = str_from_buffer(&output);
    fprintf(stderr, "%*s\n", (int)output_view.size, output_view.start);
    arena_reset(&diagnostics_arena);
  }
}
