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

static void write_diagnostic_position_indicator(StringBuffer* output,
                                                SourceRange error_range,
                                                uint32_t line_begin,
                                                uint32_t line_end);

void write_diagnostics(StringBuffer* output, const Error* error,
                       const DiagnosticsContext* context)
{
  const char* file_path = context->filename;
  const StringView source = context->source;
  const LineNumTable* line_num_table = context->line_num_table;

  const StringView msg = error->msg;
  const SourceRange error_range = error->range;

  const LineColumn begin_line_column =
      calculate_line_and_column(line_num_table, error_range.begin);
  const LineColumn end_line_column =
      calculate_line_and_column(line_num_table, error_range.end);

  string_buffer_printf(output, "%s:%u:%u: Error: %.*s\n", file_path,
                       begin_line_column.line, begin_line_column.column,
                       (int)msg.size, msg.start);

  MCC_ASSERT(end_line_column.column != 0);

  uint32_t end_line_num = (end_line_column.column == 1)
                              ? end_line_column.line - 1
                              : end_line_column.line;

  for (uint32_t line_num = begin_line_column.line; line_num <= end_line_num;
       ++line_num) {
    MCC_ASSERT(line_num != 0);
    const uint32_t line_begin = line_num_table->line_starts[line_num - 1];
    const uint32_t line_end = line_num_table->line_starts[line_num];

    const uint32_t line_length = line_end - line_begin;

    string_buffer_printf(output, "%d | %.*s", line_num, (int)line_length,
                         source.start + line_begin);
    write_diagnostic_position_indicator(output, error_range, line_begin,
                                        line_end);
  }
}

static void write_diagnostic_position_indicator(StringBuffer* output,
                                                SourceRange error_range,
                                                uint32_t line_begin,
                                                uint32_t line_end)
{
  string_buffer_printf(output, "  | ");
  MCC_ASSERT(error_range.begin >= line_begin);
  MCC_ASSERT(error_range.end > error_range.begin);
  MCC_ASSERT(error_range.end <= line_end);

  for (uint32_t i = line_begin; i < error_range.begin; ++i) {
    string_buffer_printf(output, " ");
  }
  string_buffer_printf(output, "^");
  for (uint32_t i = error_range.begin + 1; i < error_range.end; ++i) {
    string_buffer_printf(output, "~");
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
    (void)fprintf(stderr, "%*s\n", (int)output_view.size, output_view.start);
    arena_reset(&diagnostics_arena);
  }
}
