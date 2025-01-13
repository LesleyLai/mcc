#include <mcc/arena.h>

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

/**
 * @brief Attempts to extends the memory block pointed by `old_p`, or allocate a
 * new memory block if `old_p` is null
 * @pre The old size is smaller than the new size
 * @pre The arena has enough memory to allocate
 *
 * @warning If the old size does not match the actual size of the allocation
 * pointed by `old_p`, the behavior is undefined. Similarly, if the specified
 * alignment does not mathc the actual alignment
 *
 * The growth is done either by:
 * - extending the existing area of `old_p` when possible. The content of the
 * old part of the area is unchanged, and the content of the new part of the
 * area is undefined.
 * - Allocating a new chunk of memory and copying memory area of the old
 * block to there.
 *
 * This function always perform reallocation if old_p does not satisfy the
 * alignment requirement of `alignment`.
 *
 * If successful, returns a pointer to the new allocated block. The memory
 * pointed by `old_p` should be considered inaccessible afterward.
 */
void* arena_aligned_realloc(Arena* arena, void* old_p, size_t alignment,
                            size_t old_size, size_t new_size)
{
  if (old_p == NULL) { return arena_aligned_alloc(arena, alignment, new_size); }

  if (old_p != arena->previous) {
    // old_p does not point to the latest allocation of arena, reallocate
    void* new_p = arena_aligned_alloc(arena, alignment, new_size);
    memcpy(new_p, old_p, old_size);
    return new_p;
  }

  MCC_ASSERT_MSG(arena->size_remain >= new_size, "arena is too small");

  Byte* aligned_ptr = align_forward(arena->previous, alignment);
  MCC_ASSERT(old_size == (size_t)(arena->current - arena->previous));
  MCC_ASSERT_MSG(old_size < new_size, "Old size is too small");

  if (old_p != aligned_ptr) {
    // can't extend previous allocation, realloc
    void* new_p = arena_aligned_alloc(arena, alignment, new_size);
    memcpy(new_p, old_p, old_size);
    return new_p;
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
