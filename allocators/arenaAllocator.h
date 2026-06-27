#if !defined ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_H (1)
  #include "../allocator.h"

AllocatorV arena_new_ext(AllocatorV allocator, usize blocksize);
void arena_clear(AllocatorV allocator);
usize arena_countBlocks(AllocatorV allocator);
void arena_cleanup(AllocatorV allocator);
usize arena_totalMem(AllocatorV allocator);
usize arena_footprint(AllocatorV allocator);

#endif
#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define ARENA_ALLOCATOR_C (1)
#endif

#if defined(ARENA_ALLOCATOR_C)
#endif
