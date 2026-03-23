#ifndef MY_DEBUG_ALLOCATOR_H
#define MY_DEBUG_ALLOCATOR_H
#include "allocator.h"
#include "assertMessage.h" // debug symbols
#include "hhmap.h"
#include "macros.h"
#include "mytypes.h"
#include "print.h"
#include <stdlib.h>

struct dbgAlloc_config {
  AllocatorV allocator;
  bool track_total;
  bool track_trace;
  FILE *log;
};
/**
 * `@param` **allocator**  allocator
 *      - backend allocator, it will also store itself here
 * `@return` debug allocator
 */
AllocatorV debugAllocatorInit(struct dbgAlloc_config config);
struct debugStats {
  isize max_memory, current_memory, total_calls;
};
struct debugStats debugAllocator_stats(AllocatorV allocator);
#define PREPEND_DOT_MAC(...) .__VA_ARGS__,
#define debugAllocator(...) ({                     \
  struct dbgAlloc_config config = {                \
      APPLY_N(PREPEND_DOT_MAC, __VA_ARGS__)        \
  };                                               \
  config.allocator = config.allocator ?: stdAlloc; \
  debugAllocatorInit(config);                      \
})

/**
 * `@param` **allocator**  allocator
 *      - debug allocator
 * `@return` number of leaks found
 *      - will free itself along with any leaks it finds
 *      - will print traces to stdout
 */
int debugAllocatorDeInit(AllocatorV);
// #include "tests.cpp"

#if defined(MAKE_TEST_FN)
MAKE_TEST_FN(debug_allocator_test, {
  usize allocations = 5;
  usize total = 0;
  AllocatorV debug = debugAllocator(
      allocator = allocator, track_total = 1
  );
  // defer { debugAllocatorDeInit(debug); };
  for (each_RANGE(usize, i, 0, allocations)) {
    usize size = (i * i) + 1;
    aResize(debug, aCreate(debug, int, 1), size);
    total += size;
  }
  auto stats = debugAllocator_stats(debug);
  if (!EQUAL_ALL(stats.max_memory, stats.current_memory, total))
    return 1;
  if (debugAllocatorDeInit(debug) != allocations)
    return 1;
  return 0;
});
#endif

#endif // MY_DEBUG_ALLOCATOR_H

#if defined(MY_DEBUG_ALLOCATOR_C) && MY_DEBUG_ALLOCATOR_C == (1)
#define MY_DEBUG_ALLOCATOR_C (2)
#include "src/debugallocator.c"
#endif // MY_DEBUG_ALLOCATOR_C
