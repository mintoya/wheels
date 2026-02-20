#include <stddef.h>
#ifndef ARENA_ALLOCATOR_H
  #define ARENA_ALLOCATOR_H
  #include "mytypes.h"
  //
  #include "allocator.h"
  #include "assertMessage.h"
  #include "print.h"
  #include <assert.h>
  #include <errno.h>
  #include <stdint.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>

OwnAllocator arena_owned_new(void);
My_allocator *arena_new_ext(AllocatorV base, size_t blockSize);
void arena_cleanup(My_allocator *arena);
void arena_clear(My_allocator *arena);
size_t arena_footprint(My_allocator *arena);
static void arena_cleanup_handler(My_allocator **arenaPtr) {
  if (arenaPtr && *arenaPtr) {
    arena_cleanup(*arenaPtr);
    *arenaPtr = NULL;
  }
}
void my_arena_print_allocs(My_allocator *arena);
My_allocator *ownArenaInit(void);
void ownArenaDeInit(My_allocator *);
static OwnAllocator arena_owned = {ownArenaInit, ownArenaDeInit};
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

  assertMessage(res != MAP_FAILED, "mremap failed oom");

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
  // #error couldnt find page allocator
    #define pageAllocatorNotFound (1)
  #endif

  #if !defined(pageAllocatorNotFound)
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
AllocatorV pageAllocator = (My_allocator[]){{allocatePage, freePage, reallocatePage, getPageSize}};
  #endif
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
  return arena_new_ext(pageAllocator, 1024);
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
  usize *lastSize = (usize *)((uint8_t *)ptr - alignof(max_align_t));
  if (it->buffer + it->place == (u8 *)ptr + *lastSize) {
    it->place -= *lastSize + alignof(max_align_t);
  } else
    *lastSize *= -1;
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
  return res;
}
void my_arena_print_allocs(My_allocator *arena) {
  ArenaBuf *it = ((ArenaHead *)(arena->arb))->next;
  int chunk_idx = 0;

  print_("--- Arena Allocation Map ---\n");
  while (it) {
    print_("Chunk {}: ptr={ptr}, cap={}, used={}\n", chunk_idx++, (void *)it, it->size, it->place);

    usize offset = 0;
    int alloc_count = 0;

    // Walk the buffer until we hit the current 'place'
    while (offset < it->place) {
      // Read the size header stored at the current offset
      isize raw_size = *(isize *)(it->buffer + offset);

      // Handle your *lastSize *= -1 logic from my_arena_free
      bool is_free = (raw_size < 0);
      usize actual_size = is_free ? (usize)(-raw_size) : (usize)raw_size;

      print_(
          "  [{}] Offset: {} | Size: {} | Status: {cstr} | Ptr: {ptr}\n",
          alloc_count++,
          offset + alignof(max_align_t),
          actual_size,
          is_free ? "FREE" : "ACTIVE",
          (void *)(it->buffer + offset + alignof(max_align_t))
      );

      // Jump to the next allocation:
      // current offset + header space + the size of the data
      offset += alignof(max_align_t) + actual_size;

      // Safety: if a header is 0 (uninitialized memory), break to avoid infinite loop
      if (actual_size == 0 && offset < it->place) {
        print_("  !! Error: Zero-size allocation detected at offset {}\n", offset);
        break;
      }
    }
    it = it->next;
  }
  print_("----------------------------\n");
}
#endif // ARENA_ALLOCATOR_C
