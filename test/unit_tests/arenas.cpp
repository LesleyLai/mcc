#include "arenas.hpp"

Arena& get_permanent_arena()
{
  thread_local Arena arena = arena_from_virtual_mem(1024);
  return arena;
}

Arena get_scratch_arena()
{
  thread_local Arena arena = arena_from_virtual_mem(1024);
  return arena;
}