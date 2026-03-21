#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H
#include "allocator.h"
#include "assertMessage.h"
#include "mytypes.h"
#include <string.h>
#if HAS_ASAN
  #include <sanitizer/asan_interface.h>
#else
  #define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr, size))
  #define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr, size))
#endif

// create arena based on another arena
AllocatorV arena_new_ext(AllocatorV base, usize blockSize);
// free's arena
void arena_cleanup(AllocatorV arena);
// reset's the arena blocks but doesnt free anything
void arena_clear(AllocatorV arena);
// total bytes taken up by the arena
usize arena_footprint(My_allocator *arena);
static void arena_cleanup_handler [[maybe_unused]] (My_allocator **arenaPtr) {
  if (arenaPtr && *arenaPtr) {
    arena_cleanup(*arenaPtr);
    *arenaPtr = NULL;
  }
}
usize arena_totalMem(AllocatorV arena);

#define Arena_scoped [[gnu::cleanup(arena_cleanup_handler)]] My_allocator

#if defined(MAKE_TEST_FN)
  #include "macros.h"
MAKE_TEST_FN(arena_test, {
  AllocatorV arena = arena_new_ext(allocator, 1);
  defer { arena_cleanup(arena); };
  int *ints = aCreate(arena, int, 5);
  int *ints2 = aCreate(arena, int, 67);
  aFree(arena, ints2);
  aFree(arena, ints);
  usize r = arena_totalMem(arena);
  if (r)
    return 1;
  ints = aCreate(arena, int, 5);
  arena_clear(arena);
  r = arena_totalMem(arena);
  if (r)
    return 1;
  return 0;
});

#endif

#endif // ARENA_ALLOCATOR_H

#if defined(ARENA_ALLOCATOR_C) && ARENA_ALLOCATOR_C == (1)
#define ARENA_ALLOCATOR_C (2)
#include "src/arenaAllocator.c"
#endif // ARENA_ALLOCATOR_C
