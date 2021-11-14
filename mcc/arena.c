#include "arena.h"

#include <stdint.h>

// Given a pointer ptr, returns a pointer aligned by the specified alignment
// The returned pointer always have higher address than the original pointer
static Byte* align_forward(Byte* ptr, size_t alignment)
{
  const uintptr_t addr = (uintptr_t)ptr;
  const uintptr_t aligned_addr =
      (addr + (alignment - 1)) & -alignment; // Round up to align-byte boundary
  return ptr + (aligned_addr - addr);
}

void* arena_aligned_alloc(Arena* arena, size_t alignment, size_t size)
{
  Byte* aligned_ptr = align_forward(arena->ptr, alignment);
  const size_t size_for_alignment = aligned_ptr - arena->ptr;
  const size_t bump_size = size_for_alignment + size;
  if (arena->size_remain < bump_size) { return NULL; }

  arena->ptr = aligned_ptr + size;
  arena->size_remain -= bump_size;
  return aligned_ptr;
}
