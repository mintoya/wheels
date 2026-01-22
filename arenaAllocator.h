#include <stddef.h>
#define _GNU_SOURCE
#ifndef ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_H
  #include "allocator.h"
  #include "assertMessage.h"
  #include <assert.h>
  #include <errno.h>
  #include <stdint.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>

OwnAllocator arena_owned_new(void);
My_allocator *arena_new_ext(AllocatorV base, size_t blockSize);
void arena_cleanup(My_allocator *arena);
size_t arena_footprint(My_allocator *arena);
static void arena_cleanup_handler(My_allocator **arenaPtr) {
  if (arenaPtr && *arenaPtr) {
    arena_cleanup(*arenaPtr);
    *arenaPtr = NULL;
  }
}
My_allocator *ownArenaInit(void);
void ownArenaDeInit(My_allocator *);
OwnAllocator arena_owned = {ownArenaInit, ownArenaDeInit};
extern AllocatorV pageAllocator;

  #define Arena_scoped [[gnu::cleanup(arena_cleanup_handler)]] My_allocator
#endif // ARENA_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define ARENA_ALLOCATOR_C (1)
#endif
#ifdef ARENA_ALLOCATOR_C

  #include "fptr.h"
  #define PAGESIZE

void *getPage(usize);
void returnPage(void *);
  #if defined(__linux__) || defined(__APPLE__)
    #include <sys/mman.h>
    #include <unistd.h>
    #undef PAGESIZE
    #define PAGESIZE ((usize)sysconf(_SC_PAGESIZE))
void *getPage(usize size) {
  usize pagesize = PAGESIZE;
  size = lineup(size + alignof(max_align_t), pagesize);
  void *res =
      mmap(
          NULL, size,
          PROT_READ | PROT_WRITE,
          MAP_PRIVATE | MAP_ANON,
          -1, 0
      );
  assertMessage((uptr)res != (uptr)MAP_FAILED);
  static_assert(sizeof(usize) < alignof(max_align_t));
  *(usize *)res = size;
  res = (u8 *)res + alignof(max_align_t);
  return res;
}
void returnPage(void *page) {
  assertMessage(page);
  usize pagesize = PAGESIZE;
  page = ((u8 *)page - alignof(max_align_t));
  assertMessage(!((uintptr_t)page % pagesize), "unalgned page pointer %p ", page);
  assertMessage(
      munmap(page, *(usize *)page) != -1,
      "\tpagesize:%lu\n"
      "\tpointer :%p\n"
      "\tsterror() -> %s",
      pagesize,
      page,
      strerror(errno)
  );
}
void *movePage(void *page, usize size) {
  void *real_ptr = (uint8_t *)page - alignof(max_align_t);
  usize old_size = *(usize *)real_ptr;
  usize pagesize = PAGESIZE;
  size = lineup(size, pagesize);

  void *res = mremap(real_ptr, old_size, size, MREMAP_MAYMOVE);

  assertMessage(res == MAP_FAILED, "mremap failed: system out of memory");

  *(usize *)res = size;
  return (uint8_t *)res + alignof(max_align_t);
}
  #elif defined(_WIN32)
    // #if !defined(_WIN32_LEAN_AND_MEAN)
    //   #define _WIN32_LEAN_AND_MEAN
    // #endif
    #include <windows.h>
    #undef PAGESIZE
    #define PAGESIZE ({ SYSTEM_INFO si; GetSystemInfo(&si); si.dwPageSize; })
void *getPage(usize size) {
  usize pagesize = PAGESIZE;
  size = lineup(size + alignof(max_align_t), pagesize);
  void *res = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  assertMessage(res);
  *(usize *)res = size;
  return (uint8_t *)res + alignof(max_align_t);
}
void returnPage(void *page) {
  assertMessage(page);
  page = ((uint8_t *)page - alignof(max_align_t));
  usize pagesize = *(usize *)page;
  assertMessage(!((uintptr_t)page % pagesize), "unalgned page pointer");

  BOOL result = VirtualFree(page, 0, MEM_RELEASE);
  if (!result) {
    LPVOID msg;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        0,
        GetLastError(),
        0,
        (LPSTR)&msg,
        0,
        0
    );
    assertMessage(
        false,
        "\tpagesize:%llu\n"
        "\tpointer :%p\n"
        "\tgetlasterror ->() %s",
        pagesize,
        page,
        (char *)msg
    );
  }
}
void *movePage(void *page, usize size) {
  usize *realsize = (usize *)((uptr)page - alignof(max_align_t));
  if (*realsize >= size)
    return page;
  void *result = getPage(size);
  memcpy(result, page, *realsize);
  returnPage(page);
  return result;
}

  #else
    #error couldnt find page allocator
  #endif

void *allocatePage(AllocatorV _, usize size) { return getPage(size); }
void freePage(AllocatorV _, void *page) { return returnPage(page); }
void *reallocatePage(AllocatorV _, void *page, usize size) { return movePage(page, size); }
usize getPageSize(AllocatorV _, void *page) {
  assertMessage(page);
  usize pagesize = PAGESIZE;
  page = ((uint8_t *)page - alignof(max_align_t));
  assertMessage(!((uintptr_t)page % pagesize), "unalgned page pointer");
  return *(usize *)page;
}
void my_arena_free(AllocatorV arena, void *ptr);
void *my_arena_alloc(AllocatorV arena, usize size);
void *my_arena_r_alloc(AllocatorV arena, void *ptr, usize size);
usize my_arena_realsize(AllocatorV arena, void *ptr);

const My_allocator pageAllocatorVal = (My_allocator){allocatePage, freePage, reallocatePage, NULL, getPageSize};
AllocatorV pageAllocator = &pageAllocatorVal;
typedef struct ArenaBlock ArenaBlock;
typedef struct ArenaBlock {
  ArenaBlock *next;
  usize place;
  usize size;
  u8 *buffer;
  AllocatorV allocator;
} ArenaBlock;

ArenaBlock *arenablock_new(AllocatorV allocator, usize blockSize) {
  usize headerOffset = lineup(sizeof(ArenaBlock), alignof(max_align_t));
  usize fullsize = headerOffset + blockSize;

  ArenaBlock *res = (ArenaBlock *)aAlloc(allocator, fullsize);
  if (!res)
    return NULL;

  if (allocator->size)
    fullsize = allocator->size(allocator, res);

  *res = (ArenaBlock){
      .next = NULL,
      .place = 0,
      .size = fullsize - headerOffset,
      .buffer = (uint8_t *)res + headerOffset,
      .allocator = allocator,
  };
  return res;
}
usize arena_totalMem(My_allocator *arena) {
  usize res = 0;
  ArenaBlock *it = (ArenaBlock *)(arena->arb);
  AllocatorV allocator = it->allocator;
  while (it) {
    ArenaBlock *next = it->next;
    res += it->place;
    it = next;
  }
  return res;
}
usize arena_footprint(My_allocator *arena) {
  usize res = 0;
  ArenaBlock *it = (ArenaBlock *)(arena->arb);
  AllocatorV allocator = it->allocator;
  while (it) {
    ArenaBlock *next = it->next;
    res += it->size;
    it = next;
  }
  return res;
}

My_allocator *ownArenaInit(void) {
  return arena_new_ext(pageAllocator, 1024);
}
void ownArenaDeInit(My_allocator *d) {
  return arena_cleanup(d);
}
My_allocator *arena_new_ext(AllocatorV base, usize blockSize) {
  My_allocator *res = (My_allocator *)aAlloc(base, sizeof(My_allocator));
  *res = (My_allocator){
      my_arena_alloc,
      my_arena_free,
      my_arena_r_alloc,
      NULL,
      my_arena_realsize,
  };
  res->arb = arenablock_new(base, blockSize);

  return res;
}
void arena_cleanup(My_allocator *arena) {
  ArenaBlock *it = (ArenaBlock *)(arena->arb);
  const My_allocator allocator = *(it->allocator);
  while (it) {
    ArenaBlock *next = it->next;
    aFree(&allocator, it);
    it = next;
  }
  aFree(&allocator, arena);
}
bool inarena(ArenaBlock *it, const void *ptr) {
  return (uintptr_t)ptr > (uintptr_t)it->buffer && (uintptr_t)ptr < (uintptr_t)it->buffer + (uintptr_t)it->size;
}
void my_arena_free(AllocatorV allocator, void *ptr) {
  ArenaBlock *it = (ArenaBlock *)(allocator->arb);
  while (it && !inarena(it, ptr))
    it = it->next;
  assertMessage(it, "ptr %p not in arena", ptr);
  usize *lastSize = (usize *)((uint8_t *)ptr - alignof(max_align_t));
  if (it->buffer + it->place == (u8 *)ptr + *lastSize)
    it->place -= *lastSize + alignof(max_align_t);
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

  ArenaBlock *it = (ArenaBlock *)(arena->arb);
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
  ArenaBlock *it = (ArenaBlock *)(ref->arb);
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
        int minsize = size + alignof(max_align_t) * 2 + sizeof(ArenaBlock);
        minsize = (minsize < it->size ? it->size : minsize);
        it->next = arenablock_new(it->allocator, minsize);
      }
      it = it->next;
    }
  }
  return res;
}
void *arena_startStack(AllocatorV ref) {
  ArenaBlock *it = (ArenaBlock *)(ref->arb);
  while (it->next)
    it = it->next;
  return it->buffer + it->place;
}
void arena_endStack(AllocatorV ref, void *pointer) {
  ArenaBlock *it = (ArenaBlock *)(ref->arb);
  while (it && !((uintptr_t)it->buffer < (uintptr_t)pointer && (uintptr_t)it->buffer + it->size >= (uintptr_t)pointer))
    it = it->next;
  assertMessage(it);
  ArenaBlock *freeing = it->next;
  while (freeing) {
    ArenaBlock *next = freeing->next;
    aFree(freeing->allocator, freeing);
    freeing = next;
  }
  it->place = (uintptr_t)pointer - (uintptr_t)it->buffer;
}
#endif // ARENA_ALLOCATOR_C
