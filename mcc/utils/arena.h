#ifndef MCC_ARENA_H
#define MCC_ARENA_H

#include "prelude.h"
#include <stddef.h>

typedef struct Arena Arena;

typedef unsigned char Byte;

// Arena memory allocator for bulk allocations
typedef struct Arena {
  void* begin;
  Byte* previous;
  Byte* current;
  size_t size_remain;
} Arena;

Arena arena_init(void* buffer, size_t size);
void arena_reset(Arena* arena);

#if defined(__GNUC__) || defined(__clang__)
__attribute((malloc))
#endif
void* arena_aligned_alloc(Arena* arena, size_t alignment, size_t size);

void* arena_aligned_grow(Arena* arena, void* p, size_t new_alignment,
                         size_t new_size);

#define ARENA_ALLOC_OBJECT(arena, Type)                                        \
  (Type*)arena_aligned_alloc((arena), alignof(Type), sizeof(Type))

#define ARENA_ALLOC_ARRAY(arena, Type, n)                                      \
  (Type*)arena_aligned_alloc((arena), alignof(Type), sizeof(Type) * (n))

#define ARENA_GROW_ARRAY(arena, Type, p, n)                                    \
  (Type*)arena_aligned_grow((arena), (p), alignof(Type), sizeof(Type) * (n))

#endif // MCC_ARENA_H
