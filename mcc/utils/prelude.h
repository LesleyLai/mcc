#ifndef MCC_PRELUDE_H
#define MCC_PRELUDE_H

#include <stdio.h>

// Polyfill for missing stdalign.h in msvc
#if !defined(__cplusplus) && !defined(alignof)
#define alignof _Alignof
#define alignas _Alignas
#endif

// Circumvent C/C++ incompatibility of compound literal
#ifdef __cplusplus
#define MCC_COMPOUND_LITERAL(T) T
#else
#define MCC_COMPOUND_LITERAL(T) (T)
#endif

#define MCC_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define MCC_ASSERT_MSG(condition, message)                                     \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf("mcc fatal error:\n[%s:%i] Assert failed in %s: %s\n\n",          \
             __FILE__, __LINE__, __func__, message);                           \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#endif // MCC_PRELUDE_H
