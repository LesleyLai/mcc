#include <mcc/dynarray.h>
#include <mcc/frontend.h>
#include <string.h>

struct U32DynArray {
  size_t length;
  size_t capacity;
  uint32_t* data;
};

static bool table_initialized = false;
static const char* cached_file_name = nullptr;
static LineNumTable cached_table = {};

static LineNumTable create_line_num_table(StringView src,
                                          Arena* permanent_arena,
                                          Arena scratch_arena);

const LineNumTable* get_line_num_table(const char* filename, StringView source,
                                       Arena* permanent_arena,
                                       Arena scratch_arena)
{
  if (cached_file_name == nullptr) {
    cached_file_name = filename;
  } else {
    MCC_ASSERT_MSG(strcmp(filename, cached_file_name) == 0,
                   "We currently only support one file");
  }

  if (!table_initialized) {
    cached_table =
        create_line_num_table(source, permanent_arena, scratch_arena);
    table_initialized = true;
  }

  return &cached_table;
}

static LineNumTable create_line_num_table(StringView src,
                                          Arena* permanent_arena,
                                          Arena scratch_arena)
{
  struct U32DynArray line_starts_temp = {};
  DYNARRAY_PUSH_BACK(&line_starts_temp, uint32_t, &scratch_arena, 0);

  MCC_ASSERT(src.size <= UINT32_MAX);
  for (uint32_t i = 0; i < src.size; ++i) {
    if (src.start[i] == '\n') {
      DYNARRAY_PUSH_BACK(&line_starts_temp, uint32_t, &scratch_arena, i + 1);
    }
  }

  const uint32_t line_count = u32_from_usize(line_starts_temp.length);

  uint32_t* line_starts =
      ARENA_ALLOC_ARRAY(permanent_arena, uint32_t, line_count);
  memcpy(line_starts, line_starts_temp.data, line_count * sizeof(uint32_t));

  return (LineNumTable){
      .line_starts = line_starts,
      .line_count = line_count,
  };
}

static uint32_t find_line_number(const LineNumTable* table, uint32_t offset)
{
  // binary search the index of first element greater than the offset, which
  // will be the offset to the line_starts + 1 (and incidentally the line
  // number since the line number starts from 1)
  uint32_t first = 0;
  uint32_t count = table->line_count;
  uint32_t current;
  uint32_t step;

  while (count > 0) {
    step = count / 2;
    current = first + step;

    if (table->line_starts[current] <= offset) {
      first = current + 1;
      count -= step + 1;
    } else {
      count = step;
    }
  }
  return first;
}

LineColumn calculate_line_and_column(const LineNumTable* table, uint32_t offset)
{
  const uint32_t line_number = find_line_number(table, offset);
  MCC_ASSERT(line_number > 0 && line_number <= table->line_count);
  const uint32_t column_number =
      (offset - table->line_starts[line_number - 1]) + 1;
  return (LineColumn){
      .line = line_number,
      .column = column_number,
  };
}
