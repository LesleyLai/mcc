#ifndef MCC_DEFER_H
#define MCC_DEFER_H

#define MCC_MACRO_VAR(name) concat(name, __LINE__)
#define MCC_DEFER(start, end) for ( \
    int MCC_MACRO_VAR(_i_) = (start, 0);\
    !MCC_MACRO_VAR(_i_);                \
    ((MCC_MACRO_VAR(_i_) += 1), end)

#endif // MCC_DEFER_H
