#ifndef MCC_PRELUDE_H
#define MCC_PRELUDE_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

#define MCC_PANIC(message)                                                     \
  do {                                                                         \
    (void)fprintf(stderr, "mcc fatal error:\n[%s:%i] in %s: %s\n\n", __FILE__, \
                  __LINE__, __func__, message);                                \
    abort();                                                                   \
  } while (0)

#define MCC_ASSERT(condition)                                                  \
  do {                                                                         \
    if (!(condition)) {                                                        \
      (void)fprintf(stderr,                                                    \
                    "mcc fatal error:\n[%s:%i] Assert failed in %s\n\n",       \
                    __FILE__, __LINE__, __func__);                             \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define MCC_ASSERT_MSG(condition, message)                                     \
  do {                                                                         \
    if (!(condition)) {                                                        \
      (void)fprintf(stderr,                                                    \
                    "mcc fatal error:\n[%s:%i] Assert failed in %s: %s\n\n",   \
                    __FILE__, __LINE__, __func__, message);                    \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define MCC_UNIMPLEMENTED()                                                    \
  do {                                                                         \
    (void)fprintf(stderr, "mcc fatal error:\n[%s:%i]: Unimplemented\n\n",      \
                  __FILE__, __LINE__);                                         \
    abort();                                                                   \
  } while (0)

#define MCC_UNREACHABLE()                                                      \
  do {                                                                         \
    (void)fprintf(stderr, "mcc fatal error:\n[%s:%i]: unreachable\n\n",        \
                  __FILE__, __LINE__);                                         \
    abort();                                                                   \
  } while (0)

/* Defer
 *
 * Example:
 *  ```c
 *  File* file = ...
 *  MCC_DEFER(fclose(file)) {
 *    // do stuff here
 *  }
 *  ```
 */
#define MCC_CONCAT_IMPL(x, y) x##y
#define MCC_CONCAT(x, y) MCC_CONCAT_IMPL(x, y)

#define MCC_MACRO_VAR(name) MCC_CONCAT(name, __LINE__)
#define MCC_DEFER(end)                                                         \
  for (int MCC_MACRO_VAR(_i_) = 0; !MCC_MACRO_VAR(_i_);                        \
       ((MCC_MACRO_VAR(_i_) += 1), end))

/// @brief Safe cast size_t to uint32_t
inline static uint32_t u32_from_usize(size_t v)
{
  if (v > UINT_MAX) { MCC_PANIC("out of range"); }
  return (uint32_t)(v);
}

/// @brief Safe cast intptr_t to uint32_t
inline static uint32_t u32_from_isize(intptr_t v)
{
  if (v < 0) { MCC_PANIC("out of range"); }
  if (v > UINT_MAX) { MCC_PANIC("out of range"); }
  return (uint32_t)(v);
}

#endif // MCC_PRELUDE_H
