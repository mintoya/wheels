#if defined(TSA_ALLOCATOR_H)  && !defined(TSA_ALLOCATOR_C)
  #define TSA_ALLOCATOR_C (1)
  #include "tsaAllocator.h"
#endif
#undef TSA_ALLOCATOR_C

#if defined(ARENA_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)  && !defined(ARENA_ALLOCATOR_C)
  #define ARENA_ALLOCATOR_C (1)
  #include "arenaAllocator.h"
#endif
#undef ARENA_ALLOCATOR_C

#if defined(FBA_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)  && !defined(FBA_ALLOCATOR_C)
  #define FBA_ALLOCATOR_C (1)
  #include "fbaAllocator.h"
#endif
#undef FBA_ALLOCATOR_C

#if defined(MY_DEBUG_ALLOCATOR_H) || defined(WHEELS_INCLUDE_ALL)  && !defined(MY_DEBUG_ALLOCATOR_C)
  #define MY_DEBUG_ALLOCATOR_C (1)
  #include "debugallocator.h"
#endif
#undef MY_DEBUG_ALLOCATOR_C
