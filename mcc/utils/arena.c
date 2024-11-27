#include "arena.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Given a pointer current, returns a pointer aligned by the specified alignment
// The returned pointer always have higher address than the original pointer
static Byte* align_forward(Byte* ptr, size_t alignment)
{
  const uintptr_t addr = (uintptr_t)ptr;
  const uintptr_t aligned_addr =
      (addr + (alignment - 1)) & -alignment; // Round up to align-byte boundary
  return ptr + (aligned_addr - addr);
}

/**
  Allocate size bytes of uninitialized storage whose alignment is specified
  by alignment from the arena. The size parameter must be an integral multiple
  of alignment. If the arena doesn't have enough memory, returns NULL
 */
void* arena_aligned_alloc(Arena* arena, size_t alignment, size_t size)
{
  Byte* aligned_ptr = align_forward(arena->current, alignment);
  const size_t size_for_alignment = (size_t)(aligned_ptr - arena->current);
  const size_t bump_size = size_for_alignment + size;

  MCC_ASSERT_MSG(arena->size_remain >= bump_size, "arena is too small");

  arena->previous = arena->current;
  arena->current = aligned_ptr + size;
  arena->size_remain -= bump_size;
  return aligned_ptr;
}

// Reallocate and copy old data
static void* _arena_reallocate(Arena* arena, size_t new_alignment,
                               size_t old_size, size_t new_size)
{
  const void* old_p = arena->previous;
  void* new_p = arena_aligned_alloc(arena, new_alignment, new_size);
  memcpy(new_p, old_p, old_size);
  return new_p;
}

/**
 * @brief Attempts to extends the memory block pointed by `p`
 * @pre The old size is smaller than the new size
 *
 * The growth is done either by:
 * - extending the existing area of `p` when possible. The content of the
 * old part of the area is unchanged, and the content of the new part of the
 * area is undefined.
 * - Allocating a new chunk of memory and copying memory area of the old
 * block to there.
 *
 * This function always perform reallocation if p does not satisfy the
 * alignment requirement of `new_alignment`.
 *
 * If successful, returns a pointer to the new allocated block. The memory
 * pointed by `p` should be considered inaccessible afterward.
 *
 * If the arena does not have enough memory, or if `p` does not point to the
 * most recent allocated block of the arena, the function returns NULL and
 * the arena is unchanged.
 */
void* arena_aligned_grow(Arena* arena, void* p, size_t new_alignment,
                         size_t new_size)
{
  MCC_ASSERT_MSG(p == arena->previous,
                 "Can't grow a point that does not point to the most recent "
                 "allocated block of the arena");

  MCC_ASSERT_MSG(arena->size_remain >= new_size, "arena is too small");

  Byte* aligned_ptr = align_forward(arena->previous, new_alignment);
  const size_t old_size = (size_t)(arena->current - arena->previous);
  assert(old_size < new_size);

  if (p != aligned_ptr) {
    return _arena_reallocate(arena, new_alignment, old_size, new_size);
  }

  arena->size_remain = arena->size_remain + old_size - new_size;
  arena->current = aligned_ptr + new_size;
  return aligned_ptr;
}

Arena arena_init(void* buffer, size_t size)
{
  return (Arena){
      .begin = buffer,
      .previous = NULL,
      .current = (Byte*)buffer,
      .size_remain = size,
  };
}

// Reset the arena and the underlying buffer can be reused later
void arena_reset(Arena* arena)
{
  Byte* begin = (Byte*)arena->begin;
  arena->size_remain += (size_t)(arena->current - begin);
  arena->current = begin;
  arena->previous = NULL;
}