#ifndef MCC_ALLOCATORS_H
#define MCC_ALLOCATORS_H

#include <stddef.h>

typedef struct PolyAllocator PolyAllocator;

/**
 * @brief A polymorphic allocator interface
 */
typedef struct PolyAllocator {
  void* user_data;
  void* (*aligned_alloc)(const PolyAllocator* self, size_t alignment,
                         size_t size);
} PolyAllocator;

#if !defined(__cplusplus) && !defined(alignof)
#define alignof _Alignof
#endif

// Allocate size bytes of uninitialized storage. The size parameter must be an
// integral multiple of alignment.
void* poly_aligned_alloc(const PolyAllocator* allocator, size_t alignment,
                         size_t size);

#define POLY_ALLOC_OBJECT(allocator, Type)                                     \
  poly_aligned_alloc((allocator), alignof(Type), sizeof(Type))

typedef unsigned char Byte;

// Arena memory allocator for bulk allocations

typedef struct Arena {
  void* begin;
  Byte* ptr;
  size_t size_remain;
} Arena;

static inline Arena arena_init(void* buffer, size_t size)
{
  Arena arena = {
      .begin = buffer,
      .ptr = (Byte*)buffer,
      .size_remain = size,
  };
  return arena;
}

// Reset the arena and the underlying buffer can be reused later
void arena_reset(Arena* arena);

// Allocate size bytes of uninitialized storage whose alignment is specified by
// alignment from the arena. The size parameter must be an integral multiple of
// alignment. If the arena doesn't have enough memory, returns NULL
void* arena_aligned_alloc(Arena* arena, size_t alignment, size_t size);

#if !defined(__cplusplus) && !defined(alignof)
#define alignof _Alignof
#endif

#define ARENA_ALLOC_OBJECT(arena, Type)                                        \
  arena_aligned_alloc((arena), alignof(Type), sizeof(Type))

#define ARENA_ALLOC_ARRAY(arena, Type, n)                                      \
  arena_aligned_alloc((arena), alignof(Type), sizeof(Type) * (n))

PolyAllocator poly_allocator_from_arena(Arena* arena);

#endif // MCC_ALLOCATORS_H
