#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H
#include "allocator.h"
#include "assertMessage.h"
#include "macros.h"
#include "mytypes.h"
#include <stddef.h>
#include <string.h>

// create arena based on another arena
AllocatorV arena_new_ext(AllocatorV base, usize blockSize);
// free's arena
void arena_cleanup(AllocatorV arena);
// reset's the arena blocks but doesnt free anything
void arena_clear(AllocatorV arena);
// total bytes taken up by the arena
usize arena_footprint(AllocatorV);
usize arena_countBlocks(AllocatorV arena);

usize arena_totalMem(AllocatorV arena);
static void arena_cleanup_handler [[maybe_unused]] (My_allocator **arenaPtr) {
  if (arenaPtr && *arenaPtr) {
    arena_cleanup(*arenaPtr);
    *arenaPtr = NULL;
  }
}

#define Arena_scoped [[gnu::cleanup(arena_cleanup_handler)]] My_allocator

#if defined(MAKE_TEST_FN)
  #include "macros.h"
MAKE_TEST_FN(arena_test, {
  AllocatorV arena = arena_new_ext(allocator, 1);
  defer { arena_cleanup(arena); };
  int *ints = aCreate(arena, int, 5);
  int *ints2 = aCreate(arena, int, 67);
  aFree(arena, ints2, 5);
  aFree(arena, ints, 67);
  usize r = arena_totalMem(arena);
  if (r)
    return 1;
  ints = aCreate(arena, int, 5);
  arena_clear(arena);
  r = arena_totalMem(arena);
  if (r)
    return 2;
  printf("total data :  %zu\n", sizeof(int) * (67 + 5));
  printf("total blocks : %zu\n", arena_countBlocks(arena));
  printf("total footprint : %zu\n", arena_footprint(arena));
  return 0;
});

#endif

#endif // ARENA_ALLOCATOR_H

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
#define ARENA_ALLOCATOR_C (1)
#endif

#if defined(ARENA_ALLOCATOR_C)
#include "fbaAllocator.h"

void my_arena_free(AllocatorV arena, voidptr ptr, usize size, char *, usize);
voidptr my_arena_alloc(AllocatorV arena, usize size, char *, usize);

typedef struct ArenaHead ArenaHead;
typedef struct ArenaBuf ArenaBuf;
typedef struct ArenaHead {
  AllocatorV allocator;
  ArenaBuf *next;
} ArenaHead;
typedef struct ArenaBuf {
  ArenaBuf *next;
  usize capacity, offset, count;
  alignas(myAlign) u8 buffer[/*size*/];
} ArenaBuf;
typedef struct {
  My_allocator allocator[1];
  alignas(myAlign) ArenaHead block[1];
} My_arena_includeBlock;

ArenaBuf *arenablock_new(AllocatorV allocator, usize blockSize) {
  ArenaBuf *res = (ArenaBuf *)aAlloc(allocator, sizeof(ArenaBuf) + blockSize);
  usize trueSize =
      allocator->size
          ? allocator->size(allocator, res) - sizeof(ArenaBuf)
          : blockSize;

  *res = (ArenaBuf){
      .next = nullptr,
      .offset = 0,
      .count = 0,
      .capacity = trueSize,
  };
  return res;
}
usize arena_totalMem(AllocatorV arena) {
  usize res = 0;
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  while (it) {
    ArenaBuf *next = it->next;
    res += it->offset;
    it = next;
  }
  return res;
}
usize arena_footprint(AllocatorV arena) {
  usize res = 0;
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  while (it) {
    ArenaBuf *next = it->next;
    res += it->capacity;
    it = next;
  }
  return res;
}

void ownArenaDeInit(My_allocator *d) {
  return arena_cleanup(d);
}
AllocatorV arena_new_ext(AllocatorV base, usize blockSize) {
  My_arena_includeBlock *res = aCreate(base, My_arena_includeBlock);
  static const var_ defaultArena =
      (My_allocator){
          .alloc = my_arena_alloc,
          .free = my_arena_free,
          .resize = nullptr,
          .size = nullptr,
      };
  memcpy(res->allocator, &defaultArena, sizeof(defaultArena));
  res->block[0] = (typeof(*res->block)){
      .allocator = base,
      .next = arenablock_new(base, blockSize),
  };
  return res->allocator;
}
void arena_cleanup(AllocatorV arena) {
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  AllocatorV allocator = ((ArenaHead *)(arena->arb))->allocator;
  while (it) {
    ArenaBuf *next = it->next;
    aFree(allocator, it, sizeof(*it) + it->capacity);
    it = next;
  }
  aFree(allocator, (void *)arena, sizeof(My_arena_includeBlock));
}
void arena_clear(AllocatorV arena) {
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  while (it) {
    ArenaBuf *next = it->next;
    it->offset = 0;
    it = next;
  }
}
bool inarena(ArenaBuf *it, const void *ptr) {
  return (uintptr_t)ptr >= (uintptr_t)it->buffer && (uintptr_t)ptr < (uintptr_t)it->buffer + (uintptr_t)it->capacity;
}
FBA_State arena_toFBA(ArenaBuf *ab) {
  return (FBA_State){
      .capacity = ab->capacity,
      .offset = ab->offset,
      .count = ab->count,
      .allocator = {},
      .buffer = ab->buffer,
  };
}
void sync_fba(ArenaBuf *ab, FBA_State fba) {
  ab->capacity = fba.capacity;
  ab->offset = fba.offset;
  ab->count = fba.count;
}
void my_arena_free(AllocatorV arena, void *ptr, usize size, char *fn, usize line) {
  My_arena_includeBlock *maib = (typeof(maib))arena;
  ArenaBuf *b = maib->block->next;
  while (b && !inarena(b, ptr))
    b = b->next;
  assertMessage(b, "ptr not in any arena block");
  FBA_State fbs = arena_toFBA(b);
  _fba_free(fbs.allocator, ptr, size, fn, line);
  sync_fba(b, fbs);
}
void *my_arena_alloc(AllocatorV arena, usize size, char *filename, usize ln) {
  My_arena_includeBlock *maib = (typeof(maib))arena;
  ArenaBuf *b = maib->block->next;
  void *res = nullptr;
  do {
    FBA_State fbs = arena_toFBA(b);
    res = _fba_alloc_nullable(fbs.allocator, size);
    if (res) {
      sync_fba(b, fbs);
    } else {
      usize nextsize = b->capacity * 2;
      if (nextsize < size + alignof(myAlign) + alignof(myAlign))
        nextsize = size + alignof(myAlign) + alignof(myAlign);

      b->next = b->next ?: arenablock_new(maib->block[0].allocator, nextsize);
      b = b->next;
    }
  } while (!res);
  return res;
}
usize arena_countBlocks(AllocatorV arena) {
  My_arena_includeBlock *maib = (typeof(maib))arena;
  ArenaBuf *b = maib->block->next;
  usize res = 1;
  while (b->next) {
    b = b->next;
    res++;
  }
  return res;
}

#endif // ARENA_ALLOCATOR_C
