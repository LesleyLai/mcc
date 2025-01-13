#ifndef MCC_ARENA_H
#define MCC_ARENA_H

#include "prelude.h"

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

// Allocate an arena from a large chunk of OS virtual memory
Arena arena_from_virtual_mem(size_t size);

#if defined(__GNUC__) || defined(__clang__)
__attribute((malloc))
#endif
void* arena_aligned_alloc(Arena* arena, size_t alignment, size_t size);

void* arena_aligned_realloc(Arena* arena, void* old_p, size_t alignment,
                            size_t old_size, size_t new_size);

#define ARENA_ALLOC_OBJECT(arena, Type)                                        \
  (Type*)arena_aligned_alloc((arena), alignof(Type), sizeof(Type))

#define ARENA_ALLOC_ARRAY(arena, Type, n)                                      \
  (Type*)arena_aligned_alloc((arena), alignof(Type), sizeof(Type) * (n))

#define ARENA_REALLOC_ARRAY(arena, Type, old_p, old_n, new_n)                  \
  (Type*)arena_aligned_realloc((arena), (old_p), alignof(Type),                \
                               sizeof(Type) * (old_n), sizeof(Type) * (new_n))

#endif // MCC_ARENA_H
