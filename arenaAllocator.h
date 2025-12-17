#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H
#include "allocator.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

OwnAllocator arena_owned_new(void);
My_allocator *pageArena_new();
My_allocator *arena_new_ext(const My_allocator *base, size_t blockSize);
void arena_cleanup(My_allocator *arena);
size_t arena_footprint(My_allocator *arena);
static void arena_cleanup_handler(My_allocator **arenaPtr) {
  if (arenaPtr && *arenaPtr) {
    arena_cleanup(*arenaPtr);
    *arenaPtr = NULL;
  }
}
extern const My_allocator *pageAllocator;
#define Arena_scoped [[gnu::cleanup(arena_cleanup_handler)]] My_allocator
#endif // ARENA_ALLOCATOR_H
//
#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define ARENA_ALLOCATOR_C
#endif
#ifdef ARENA_ALLOCATOR_C
//

#include "fptr.h"
#define PAGESIZE

void *getPage(size_t);
void returnPage(void *);
#if defined(__linux__) || defined(__APPLE__)
  #include <sys/mman.h>
  #include <unistd.h>
  #undef PAGESIZE
  #define PAGESIZE ((size_t)sysconf(_SC_PAGESIZE))
void *getPage(size_t size) {
  size_t pagesize = PAGESIZE;
  size = lineup(size + alignof(max_align_t), pagesize);
  void *res = valElse(
      mmap(
          NULL,
          size,
          PROT_EXEC |
              PROT_READ |
              PROT_WRITE,
          MAP_PRIVATE | MAP_ANON,
          -1, 0
      ),
      MAP_FAILED
  );
  *(size_t *)res = size;
  return (uint8_t *)res + alignof(max_align_t);
}
void returnPage(void *page) {
  nullElse(page);
  size_t pagesize = PAGESIZE;
  page = ((uint8_t *)page - alignof(max_align_t));
  assertMessage(!((uintptr_t)page % pagesize), "unalgned page pointer");
  assertMessage(
      munmap(page, *(size_t *)page) != -1,
      "\tpagesize:%lu\n"
      "\tpointer :%p\n"
      "\tsterror() -> %s",
      pagesize,
      page,
      strerror(errno)
  );
}
#elif defined(_WIN32)
  // #if !defined(_WIN32_LEAN_AND_MEAN)
  //   #define _WIN32_LEAN_AND_MEAN
  // #endif
  #include <windows.h>
  #undef PAGESIZE
  #define PAGESIZE ({ SYSTEM_INFO si; GetSystemInfo(&si); si.dwPageSize; })
void *getPage(size_t size) {
  size_t pagesize = PAGESIZE;
  size = lineup(size + alignof(max_align_t), pagesize);
  void *res = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  assertMessage(res);
  *(size_t *)res = size;
  return (uint8_t *)res + alignof(max_align_t);
}
void returnPage(void *page) {
  nullElse(page);
  page = ((uint8_t *)page - alignof(max_align_t));
  size_t pagesize = *(size_t *)page;
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

#else
  #error couldnt find page allocator
#endif

void *allocatePage(const My_allocator *_, size_t size) {
  size_t pagesize = PAGESIZE;
  size = lineup(size, pagesize);
  void *res = getPage(size);
  return res;
}
void freePage(const My_allocator *_, void *page) {
  return returnPage(page);
}
void *reallocatePage(const My_allocator *_, void *page, size_t __) {
  assertMessage(false, "dont reallocate pages");
  return NULL;
}
size_t getPageSize(void *page) {
  nullElse(page);
  size_t pagesize = PAGESIZE;
  page = ((uint8_t *)page - alignof(max_align_t));
  assertMessage(!((uintptr_t)page % pagesize), "unalgned page pointer");
  return *(size_t *)page;
}
void arena_free(const My_allocator *allocator, void *ptr);
void *arena_alloc(const My_allocator *ref, size_t size);
void *arena_r_alloc(const My_allocator *arena, void *ptr, size_t size);

const My_allocator pageAllocatorVal = (My_allocator){allocatePage, freePage, reallocatePage, 0, getPageSize};
const My_allocator *pageAllocator = &pageAllocatorVal;
static const My_allocator defaultAllocaator_functions = {arena_alloc, arena_free, arena_r_alloc};
typedef struct ArenaBlock ArenaBlock;
typedef struct ArenaBlock {
  ArenaBlock *next;
  size_t place;
  size_t size;
  size_t *freelist[10];
  uint8_t *buffer;
  const My_allocator *allocator;
} ArenaBlock;
ArenaBlock *arenablock_new(const My_allocator *allocator, size_t blockSize);

My_allocator *pageArena_new() {
  size_t size = PAGESIZE;

  uint8_t *ptr = (uint8_t *)allocatePage(NULL, size);
  lmemset(ptr, size, (u8)0);

  struct anonalign {
    ArenaBlock blockpart;
    My_allocator apart;
    My_allocator result;
  };

  struct anonalign *anl = (struct anonalign *)ptr;
  assertMessage(((uptr)ptr) == ((uptr) & (anl->blockpart)));
  ptr += sizeof(struct anonalign);
  size -= sizeof(struct anonalign);

  anl->blockpart.size = size;
  anl->blockpart.buffer = ptr;

  anl->blockpart.allocator = &anl->apart;
  anl->apart = pageAllocatorVal;
  anl->apart.arb = &anl->blockpart;

  anl->result = defaultAllocaator_functions;
  anl->result.arb = &(anl->blockpart);

  return &(anl->result);
}
ArenaBlock *arenablock_new(const My_allocator *allocator, size_t blockSize) {
  size_t fullsize = sizeof(ArenaBlock) + blockSize;
  ArenaBlock *res = (ArenaBlock *)aAlloc(allocator, fullsize);
  assertMessage(res, "allocator failed");

  // if (allocator->size) {
  //   fullsize = allocator->size(res);
  // }
  size_t rest = fullsize - sizeof(ArenaBlock);

  *res = (ArenaBlock){
      .next = NULL,
      .place = 0,
      .size = rest,
      .freelist = {0},
      .buffer = (uint8_t *)res + sizeof(ArenaBlock),
      .allocator = allocator,
  };
  return res;
}
void arena_cleanup(My_allocator *arena) {
  ArenaBlock *it = (ArenaBlock *)(arena->arb);
  const My_allocator allocator = *(it->allocator);
  while (it) {
    ArenaBlock *next = it->next;
    aFree(&(allocator), it);
    it = next;
  }
}
size_t arena_footprint(My_allocator *arena) {
  size_t res = 0;
  ArenaBlock *it = (ArenaBlock *)(arena->arb);
  const My_allocator *allocator = it->allocator;
  while (it) {
    ArenaBlock *next = it->next;
    res += it->size;
    it = next;
  }
  return res;
}
void arena_free(const My_allocator *allocator, void *ptr) {
  size_t *thisSize = (size_t *)((uint8_t *)ptr - alignof(max_align_t));
  ArenaBlock *it = (ArenaBlock *)(allocator->arb);
  for (int i = 0; i < 10; i++) {
    if (!it->freelist[i]) {
      it->freelist[i] = thisSize;
      return;
    }
  }
  for (int i = 0; i < 10; i++) {
    if (*it->freelist[i] < *thisSize) {
      it->freelist[i] = thisSize;
      return;
    }
  }
  // noop
}

My_allocator *ownArenaInit(void) {
  return pageArena_new();
}
void ownArenaDeInit(My_allocator *d) {
  return arena_cleanup(d);
}
OwnAllocator arena_owned_new(void) {
  return (OwnAllocator){ownArenaInit, ownArenaDeInit};
}
My_allocator *arena_new_ext(const My_allocator *base, size_t blockSize) {
  My_allocator *res =
      (My_allocator *)aAlloc(base, sizeof(My_allocator));
  assertMessage(res);
  *res = defaultAllocaator_functions;
  res->arb = arenablock_new(base, blockSize);
  return res;
}
bool inarena(ArenaBlock *it, const void *ptr) {
  return (uintptr_t)ptr > (uintptr_t)it->buffer && (uintptr_t)ptr < (uintptr_t)it->buffer + (uintptr_t)it->size;
}
void *arena_r_alloc(const My_allocator *arena, void *ptr, size_t size) {
  size = lineup(size, alignof(max_align_t));
  assertMessage(ptr, "reallocating null pointer");
  size_t *lastSize = (size_t *)((uint8_t *)ptr - alignof(max_align_t));
  if (*lastSize > size)
    return ptr;

  ArenaBlock *it = (ArenaBlock *)(arena->arb);
  size_t *lastAllocatoinPos = (size_t *)(it->buffer + it->place);

  while (it && !(inarena(it, ptr)))
    it = it->next;
  assertMessage(it);
  if (
      lastSize + *lastSize == lastAllocatoinPos &&
      it->place + (size - *lastSize) < it->size
  ) {
    it->place += size - *lastSize;
    *lastSize = size;
    return ptr;
  }
  void *res = arena_alloc(arena, size);
  memmove(res, ptr, (*lastSize));
  arena_free(arena, ptr);
  return res;
}
void *arena_alloc(const My_allocator *ref, size_t size) {
  size = lineup(size, alignof(max_align_t));
  ArenaBlock *it = (ArenaBlock *)(ref->arb);
  void *res = NULL;
  for (int i = 0; i < 10; i++) {
    if (it->freelist[i]) {
      size_t *sizeptr = it->freelist[i];
      if (*sizeptr >= size) {
        it->freelist[i] = NULL;
        return (void *)(sizeptr + 1);
      }
    }
  }
  while (!res) {
    if (it->size - it->place >= size + alignof(max_align_t)) {
      *(size_t *)(it->buffer + it->place) = size;
      it->place += alignof(max_align_t);

      res = it->buffer + it->place;
      it->place += size;
    } else {
      if (!it->next) {
        int minsize = size + alignof(max_align_t);
        minsize = (minsize < it->size ? it->size : minsize);
        it->next = arenablock_new(it->allocator, minsize);
      }
      it = it->next;
    }
  }
  return res;
}
#endif // ARENA_ALLOCATOR_C
