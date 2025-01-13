#ifndef MCC_DYNARRAY_H
#define MCC_DYNARRAY_H

#include "arena.h"

/// @file dynarray.h
/// This file provides generic helpers for defining dynamically sized arrays

// A dynamically sized array should be a struct with the following members
// T* data
// uint32_t length
// uint32_t capacity

#define DYNARRAY_PUSH_BACK(arr, T, arena, elem)                                \
  do {                                                                         \
    if ((arr)->length == (arr)->capacity) {                                    \
      const size_t old_capacity = (arr)->capacity;                             \
      (arr)->capacity = (arr)->capacity ? (arr)->capacity * 2 : 16;            \
      (arr)->data = (T*)arena_aligned_realloc(                                 \
          (arena), (arr)->data, alignof(T), old_capacity * sizeof(T),          \
          (arr)->capacity * sizeof(T));                                        \
    }                                                                          \
    (arr)->data[(arr)->length++] = elem;                                       \
  } while (0)

#endif // MCC_DYNARRAY_H
