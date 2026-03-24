#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H
#include "allocator.h"
#include "assertMessage.h"
#include "macros.h"
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
usize arena_footprint(AllocatorV);
usize arena_countBlocks(AllocatorV arena);
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

void my_arena_free(AllocatorV arena, voidptr ptr);
voidptr my_arena_alloc(AllocatorV arena, usize size);
voidptr my_arena_r_alloc(AllocatorV arena, voidptr ptr, usize size);
usize my_arena_realsize(AllocatorV arena, voidptr ptr);

typedef struct ArenaHead ArenaHead;
typedef struct ArenaBuf ArenaBuf;
typedef struct ArenaHead {
  AllocatorV allocator;
  ArenaBuf *next;
} ArenaHead;
typedef struct ArenaBuf {
  ArenaBuf *next;
  usize capacity, offset;
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
      .offset = 0,
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
  static const __auto_type defaultArena =
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
void arena_cleanup(AllocatorV arena) {
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  AllocatorV allocator = ((ArenaHead *)(arena->arb))->allocator;
  while (it) {
    ArenaBuf *next = it->next;
    aFree(allocator, it);
    it = next;
  }
  aFree(allocator, (void *)arena);
}
void arena_clear(AllocatorV arena) {
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  while (it) {
    ArenaBuf *next = it->next;
    it->offset = 0;
    ASAN_POISON_MEMORY_REGION(it->buffer, it->capacity);
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
      .allocator = {},
      .buffer = ab->buffer,
  };
}
void sync_fba(ArenaBuf *ab, FBA_State fba) {
  ab->capacity = fba.capacity;
  ab->offset = fba.offset;
}
void my_arena_free(AllocatorV arena, void *ptr) {
  My_arena_includeBlock *maib = (typeof(maib))arena;
  ArenaBuf *b = maib->block->next;
  while (b && !inarena(b, ptr))
    b = b->next;
  assertMessage(b, "ptr not in any arena block");
  FBA_State fbs = arena_toFBA(b);
  defer { sync_fba(b, fbs); };
  _fba_free(fbs.allocator, ptr);
}
usize my_arena_realsize(AllocatorV arena, void *ptr) {
  My_arena_includeBlock *maib = (typeof(maib))arena;
  ArenaBuf *b = maib->block->next;
  while (b && !inarena(b, ptr))
    b = b->next;
  assertMessage(b, "ptr not in any arena block");
  FBA_State fbs = arena_toFBA(b);
  defer { sync_fba(b, fbs); };
  return _fba_size(fbs.allocator, ptr);
}
void *my_arena_r_alloc(AllocatorV arena, void *ptr, usize size) {
  My_arena_includeBlock *maib = (typeof(maib))arena;
  ArenaBuf *b = maib->block->next;
  while (b && !inarena(b, ptr))
    b = b->next;
  assertMessage(b, "ptr not in any arena block");
  FBA_State fbs = arena_toFBA(b);
  defer { sync_fba(b, fbs); };
  void *inplace =
      _fba_resize_nullable(fbs.allocator, ptr, size);
  if (inplace)
    return inplace;
  void *res = my_arena_alloc(arena, size);
  usize oldsize = my_arena_realsize(arena, ptr);
  memcpy(res, ptr, oldsize);
  my_arena_free(arena, ptr);
  return res;
}
void *my_arena_alloc(AllocatorV arena, usize size) {
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
      if (nextsize < size + alignof(max_align_t) + sizeof(FBA_Header))
        nextsize = size + alignof(max_align_t) + sizeof(FBA_Header);

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
