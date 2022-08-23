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
  void* (*aligned_grow)(const PolyAllocator* self, void* p,
                        size_t new_alignment, size_t new_size);
  void* (*aligned_shrink)(const PolyAllocator* self, void* p,
                          size_t new_alignment, size_t new_size);
} PolyAllocator;

#if !defined(__cplusplus) && !defined(alignof)
#define alignof _Alignof
#endif

// Allocate size bytes of uninitialized storage. The size parameter must be an
// integral multiple of alignment.
void* poly_aligned_alloc(const PolyAllocator* allocator, size_t alignment,
                         size_t size);
void* poly_aligned_grow(const PolyAllocator* allocator, void* p,
                        size_t new_alignment, size_t new_size);
void* poly_aligned_shrink(const PolyAllocator* allocator, void* p,
                          size_t new_alignment, size_t new_size);

#define POLY_ALLOC_OBJECT(allocator, Type)                                     \
  poly_aligned_alloc((allocator), alignof(Type), sizeof(Type))

typedef unsigned char Byte;

// Arena memory allocator for bulk allocations

typedef struct Arena {
  void* begin;
  Byte* previous;
  Byte* current;
  size_t size_remain;
} Arena;

static inline Arena arena_init(void* buffer, size_t size)
{
  Arena arena = {
      .begin = buffer,
      .previous = NULL,
      .current = (Byte*)buffer,
      .size_remain = size,
  };
  return arena;
}

void arena_reset(Arena* arena);
void* arena_aligned_alloc(Arena* arena, size_t alignment, size_t size);
void* arena_aligned_grow(Arena* arena, void* p, size_t new_alignment,
                         size_t new_size);
void* arena_aligned_shrink(Arena* arena, void* p, size_t new_alignment,
                           size_t new_size);

#if !defined(__cplusplus) && !defined(alignof)
#define alignof _Alignof
#endif

#define ARENA_ALLOC_OBJECT(arena, Type)                                        \
  arena_aligned_alloc((arena), alignof(Type), sizeof(Type))

#define ARENA_ALLOC_ARRAY(arena, Type, n)                                      \
  arena_aligned_alloc((arena), alignof(Type), sizeof(Type) * (n))

#define ARENA_GROW_ARRAY(arena, Type, p, n)                                    \
  arena_aligned_grow((arena), (p), alignof(Type), sizeof(Type) * (n))

#define ARENA_SHRINK_ARRAY(arena, Type, p, n)                                  \
  arena_aligned_shrink((arena), (p), alignof(Type), sizeof(Type) * (n))

PolyAllocator poly_allocator_from_arena(Arena* arena);

#endif // MCC_ALLOCATORS_H
