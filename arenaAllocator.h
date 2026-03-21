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
My_allocator *arena_new_ext(AllocatorV base, usize blockSize);
// free's arena
void arena_cleanup(My_allocator *arena);
// reset's the arena blocks but doesnt free anything
void arena_clear(My_allocator *arena);
// total bytes taken up by the arena
usize arena_footprint(My_allocator *arena);
static void arena_cleanup_handler [[maybe_unused]] (My_allocator **arenaPtr) {
  if (arenaPtr && *arenaPtr) {
    arena_cleanup(*arenaPtr);
    *arenaPtr = NULL;
  }
}

#define Arena_scoped [[gnu::cleanup(arena_cleanup_handler)]] My_allocator
#endif // ARENA_ALLOCATOR_H

#if defined(ARENA_ALLOCATOR_C) && ARENA_ALLOCATOR_C == (1)
#define ARENA_ALLOCATOR_C (2)
#include "src/arenaAllocator.c"
#endif // ARENA_ALLOCATOR_C
