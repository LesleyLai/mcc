#ifndef MCC_DEFER_H
#define MCC_DEFER_H

#define MCC_CONCAT_IMPL(x, y) x##y
#define MCC_CONCAT(x, y) MCC_CONCAT_IMPL(x, y)

#define MCC_MACRO_VAR(name) MCC_CONCAT(name, __LINE__)
#define MCC_DEFER(end)                                                         \
  for (int MCC_MACRO_VAR(_i_) = 0; !MCC_MACRO_VAR(_i_);                        \
       ((MCC_MACRO_VAR(_i_) += 1), end))

#endif // MCC_DEFER_H
