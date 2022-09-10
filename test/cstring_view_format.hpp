#ifndef MCC_CSTRING_VIEW_FORMAT_HPP
#define MCC_CSTRING_VIEW_FORMAT_HPP

extern "C" {
#include "utils/str.h"
}

#include <fmt/core.h>

template <> struct fmt::formatter<StringView> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(StringView sv, FormatContext& ctx)
  {
    string_view name{sv.start, sv.size};
    return formatter<string_view>::format(name, ctx);
  }
};

#endif // MCC_CSTRING_VIEW_FORMAT_HPP
