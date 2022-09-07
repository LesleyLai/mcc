#ifndef MCC_SOURCE_LOCATION_FORMATTER_HPP
#define MCC_SOURCE_LOCATION_FORMATTER_HPP

#include <fmt/core.h>

extern "C" {
#include "source_location.h"
}

template <> struct fmt::formatter<SourceLocation> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(SourceLocation source_location, FormatContext& ctx)
  {
    return fmt::format_to(ctx.out(), "<line: {} col: {} offset: {}>",
                          source_location.line, source_location.column,
                          source_location.offset);
  }
};

template <> struct fmt::formatter<SourceRange> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(SourceRange source_range, FormatContext& ctx)
  {
    if (source_range.begin.line != source_range.end.line) {
      return fmt::format_to(ctx.out(), "line: {}..{}", source_range.begin.line,
                            source_range.end.line);
    }

    return fmt::format_to(ctx.out(), "line:{} column: {}..{}",
                          source_range.begin.line, source_range.begin.column,
                          source_range.end.column);
  }
};

#endif // MCC_SOURCE_LOCATION_FORMATTER_HPP
