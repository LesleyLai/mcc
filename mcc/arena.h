#ifndef MCC_ARENA_H
#define MCC_ARENA_H

#include <stddef.h>

typedef unsigned char Byte;

// Arena memory allocator for bulk allocations

typedef struct Arena {
  void* begin;
  Byte* ptr;
  size_t size_remain;
} Arena;

static inline Arena arena_init(void* buffer, size_t size)
{
  Arena arena;
  arena.begin = buffer;
  arena.ptr = (Byte*)buffer;
  arena.size_remain = size;
  return arena;
}

// Allocate size bytes of uninitialized storage whose alignment is specified by
// alignment from the arena. The size parameter must be an integral multiple of
// alignment. If the arena doesn't have enough memory, returns NULL
void* arena_aligned_alloc(Arena* arena, size_t alignment, size_t size);

#endif // MCC_ARENA_H
