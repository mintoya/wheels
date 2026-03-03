#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H
#include "allocator.h"
#include "assertMessage.h"
#include "mytypes.h"
#include <string.h>
#if HAS_ASAN
  #include <sanitizer/asan_interface.h>
#else
  #define ASAN_POISON_MEMORY_REGION(addr, size)
  #define ASAN_UNPOISON_MEMORY_REGION(addr, size)
#endif

// create arena based on another arena
My_allocator *arena_new_ext(AllocatorV base, usize blockSize);
// free's arena
void arena_cleanup(My_allocator *arena);
// reset's the arena blocks but doesnt free anything
void arena_clear(My_allocator *arena);
// total bytes taken up by the arena
usize arena_footprint(My_allocator *arena);
static void arena_cleanup_handler(My_allocator **arenaPtr) {
  if (arenaPtr && *arenaPtr) {
    arena_cleanup(*arenaPtr);
    *arenaPtr = NULL;
  }
}

#define Arena_scoped [[gnu::cleanup(arena_cleanup_handler)]] My_allocator
#endif // ARENA_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define ARENA_ALLOCATOR_C (1)
#endif
#ifdef ARENA_ALLOCATOR_C

void my_arena_free(AllocatorV arena, void *ptr);
void *my_arena_alloc(AllocatorV arena, usize size);
void *my_arena_r_alloc(AllocatorV arena, void *ptr, usize size);
usize my_arena_realsize(AllocatorV arena, void *ptr);

typedef struct ArenaHead ArenaHead;
typedef struct ArenaBuf ArenaBuf;
typedef struct ArenaHead {
  AllocatorV allocator;
  ArenaBuf *next;
} ArenaHead;
typedef struct ArenaBuf {
  ArenaBuf *next;
  usize place;
  usize size;
  alignas(alignof(max_align_t)) u8 buffer[/*size*/];
} ArenaBuf;
typedef struct {
  My_allocator allocator[1];
  alignas(alignof(max_align_t)) ArenaHead block[1];
} My_arena_includeBlock;

ArenaBuf *arenablock_new(AllocatorV allocator, usize blockSize) {
  ArenaBuf *res = (ArenaBuf *)aAlloc(allocator, sizeof(ArenaBuf) + blockSize);
  usize trueSize =
      allocator->size
          ? allocator->size(allocator, res) - sizeof(ArenaBuf)
          : blockSize;

  *res = (ArenaBuf){
      .next = nullptr,
      .place = 0,
      .size = trueSize,
  };
  return res;
}
usize arena_totalMem(My_allocator *arena) {
  usize res = 0;
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  while (it) {
    ArenaBuf *next = it->next;
    res += it->place;
    it = next;
  }
  return res;
}
usize arena_footprint(My_allocator *arena) {
  usize res = 0;
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  while (it) {
    ArenaBuf *next = it->next;
    res += it->size;
    it = next;
  }
  return res;
}

My_allocator *ownArenaInit(void) {
  return arena_new_ext(stdAlloc, 1024);
}
void ownArenaDeInit(My_allocator *d) {
  return arena_cleanup(d);
}
My_allocator *arena_new_ext(AllocatorV base, usize blockSize) {
  My_arena_includeBlock *res = aCreate(base, My_arena_includeBlock);
  static constexpr auto defaultArena =
      (My_allocator){
          .alloc = my_arena_alloc,
          .free = my_arena_free,
          .resize = my_arena_r_alloc,
          .size = my_arena_realsize
      };
  memcpy(res->allocator, &defaultArena, sizeof(defaultArena));
  res->block[0] = (typeof(*res->block)){
      .allocator = base,
      .next = arenablock_new(base, blockSize),
  };
  return res->allocator;
}
void arena_cleanup(My_allocator *arena) {
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  AllocatorV allocator = ((ArenaHead *)(arena->arb))->allocator;
  while (it) {
    ArenaBuf *next = it->next;
    aFree(allocator, it);
    it = next;
  }
  aFree(allocator, arena);
}
void arena_clear(My_allocator *arena) {
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  while (it) {
    ArenaBuf *next = it->next;
    it->place = 0;
    it = next;
  }
}
bool inarena(ArenaBuf *it, const void *ptr) {
  return (uintptr_t)ptr > (uintptr_t)it->buffer && (uintptr_t)ptr < (uintptr_t)it->buffer + (uintptr_t)it->size;
}
void my_arena_free(AllocatorV allocator, void *ptr) {
  ArenaBuf *it = ((ArenaHead *)(allocator->arb))->next;
  while (it && !inarena(it, ptr))
    it = it->next;
  assertMessage(it, "ptr %p not in arena", ptr);
  isize *lastSize = (isize *)((u8 *)ptr - alignof(max_align_t));
  usize oldsize = *lastSize;
  if (it->buffer + it->place == (u8 *)ptr + *lastSize) {
    it->place -= *lastSize + alignof(max_align_t);
  } else
    *lastSize *= -1;
  ASAN_POISON_MEMORY_REGION(ptr, oldsize);
}
usize my_arena_realsize(AllocatorV arena, void *ptr) {
  usize *lastSize = (usize *)((uint8_t *)ptr - alignof(max_align_t));
  return *lastSize;
}
void *my_arena_r_alloc(AllocatorV arena, void *ptr, usize size) {
  if (!ptr)
    return my_arena_alloc(arena, size);

  size = lineup(size, alignof(max_align_t));
  usize *lastSize = (usize *)((uint8_t *)ptr - alignof(max_align_t));

  if (*lastSize >= size) {
    *lastSize = size;
    return ptr;
  }

  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  while (it && !inarena(it, ptr))
    it = it->next;
  assertMessage(it, "ptr %p not in arena", ptr);

  if (it->buffer + it->place == (u8 *)ptr + *lastSize) {
    usize additional = size - *lastSize;
    if (additional <= it->size - it->place) {
      it->place += additional;
      *lastSize = size;
      return ptr;
    }
  }

  void *res = my_arena_alloc(arena, size);
  memmove(res, ptr, *lastSize);
  my_arena_free(arena, ptr);
  return res;
}
void *my_arena_alloc(AllocatorV ref, usize size) {
  size = lineup(size, alignof(max_align_t));
  ArenaBuf *it = ((ArenaHead *)(ref->arb))->next;
  void *res = NULL;
  while (it->next)
    it = it->next;
  while (!res) {
    if (it->size - it->place >= size + alignof(max_align_t)) {
      *(usize *)(it->buffer + it->place) = size;
      it->place += alignof(max_align_t);

      res = it->buffer + it->place;
      it->place += size;
    } else {
      if (!it->next) {
        int minsize = size + alignof(max_align_t) * 2 + sizeof(ArenaBuf);
        minsize = (minsize < it->size ? it->size : minsize);
        it->next = arenablock_new(((ArenaHead *)(ref->arb))->allocator, minsize);
      }
      it = it->next;
    }
  }
  ASAN_UNPOISON_MEMORY_REGION(res, size);
  return res;
}
#endif // ARENA_ALLOCATOR_C
