#include "arena.h"

#include <sys/mman.h>

Arena arena_from_virtual_mem(size_t size)
{
  void* arena_buffer = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (arena_buffer == MAP_FAILED) {
    perror("Failed to allocate scratch arena buffer");
    exit(1);
  }
  return arena_init(arena_buffer, size);
}
