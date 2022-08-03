#ifndef MCC_SOURCE_LOCATION_FORMATTER_HPP
#define MCC_SOURCE_LOCATION_FORMATTER_HPP

#include <fmt/core.h>

extern "C" {
#include "source_location.h"
};

template <> struct fmt::formatter<SourceLocation> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(SourceLocation source_location, FormatContext& ctx)
  {
    return fmt::format_to(ctx.out(), "{}:{}", source_location.line,
                          source_location.column);
  }
};

template <> struct fmt::formatter<SourceRange> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(SourceRange source_range, FormatContext& ctx)
  {
    return fmt::format_to(ctx.out(), "from: {} to: {}", source_range.first,
                          source_range.last);
  }
};

#endif // MCC_SOURCE_LOCATION_FORMATTER_HPP
