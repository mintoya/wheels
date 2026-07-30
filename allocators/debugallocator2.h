#if !defined MY_DEBUG_ALLOCATOR_H
  #define MY_DEBUG_ALLOCATOR_H
  #include "../allocator.h"
  #include "../hxmap.h"
  #include "../print/print_pre.h"
  #include <stdio.h>

struct tracedata {
  char *fn;
  usize ln;
  usize size;
};
struct dbgAlloc_config {
  allocfn allocator;
  FILE *log;
};
/**
 * `@param` **allocator**  allocator
 *      - backend allocator, it will also store itself here
 * `@return` debug allocator
 */
allocfn debugAllocatorInit(struct dbgAlloc_config config);
struct debugStats {
  usize max_memory, current_memory, total_calls, total_active_allocations;
};
struct debugStats debugAllocator_stats(allocfn allocator);
struct debugStats debugAllocator_clear(allocfn allocator);
  #define debugAllocator(...) ({                     \
    struct dbgAlloc_config config = {                \
        __VA_ARGS__                                  \
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
int debugAllocatorDeInit(allocfn);

#endif // MY_DEBUG_ALLOCATOR_H
#if (defined MY_DEBUG_ALLOCATOR_C && MY_DEBUG_ALLOCATOR_C == 1) || \
    defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #undef MY_DEBUG_ALLOCATOR_C
  #define MY_DEBUG_ALLOCATOR_C (2)

#endif // MY_DEBUG_ALLOCATOR_C
