#include "../arenaAllocator.h"
#include "../fbaAllocator.h"
#include "../mytypes.h"
#include <stdalign.h>
#include <stddef.h>
#include <string.h>

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
usize arena_footprint(My_allocator *arena) {
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
      usize nextsize =
          b->capacity < size ? size : b->capacity;
      b->next = b->next ?: arenablock_new(arena, nextsize);
      b = b->next;
    }
  } while (!res);
  return res;
}
