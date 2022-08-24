#ifndef MCC_PRELUDE_H
#define MCC_PRELUDE_H

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

#endif // MCC_PRELUDE_H
