#include "allocators.h"

#include <stdint.h>

void* poly_aligned_alloc(const PolyAllocator* allocator, size_t alignment,
                         size_t size)
{
  return allocator->aligned_alloc(allocator, alignment, size);
}

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

void arena_reset(Arena* arena)
{
  Byte* begin = (Byte*)arena->begin;
  arena->size_remain += (arena->ptr - begin);
  arena->ptr = begin;
}

// To be used as a function pointer in PolyAllocator
static void* arena_poly_aligned_alloc(const PolyAllocator* self,
                                      size_t alignment, size_t size)
{
  return arena_aligned_alloc((Arena*)self->user_data, alignment, size);
}

PolyAllocator poly_allocator_from_arena(Arena* arena)
{
  return (PolyAllocator){.user_data = arena,
                         .aligned_alloc = arena_poly_aligned_alloc};
}