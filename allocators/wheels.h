#if defined(TSA_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define TSA_ALLOCATOR_C (1)
  #include "tsaAllocator.h"
#endif
#undef TSA_ALLOCATOR_C

#if defined(ARENA_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define ARENA_ALLOCATOR_C (1)
  #include "arenaAllocator.h"
#endif
#undef ARENA_ALLOCATOR_C

#if defined(FBA_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define FBA_ALLOCATOR_C (1)
  #include "fbaAllocator.h"
#endif
#undef FBA_ALLOCATOR_H

#if defined(MY_DEBUG_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)
  #define MY_DEBUG_ALLOCATOR_C (1)
  #include "debugallocator.h"
#endif
#undef MY_DEBUG_ALLOCATOR_C
