#ifndef MY_ALLOCATOR_H
#define MY_ALLOCATOR_H
#include <stddef.h>
#include <stdint.h>

// #define MY_ALLOCATOR_STRICTEST

[[gnu::const]]
static inline uintptr_t lineup(uintptr_t unaligned, size_t aligneder) {
  return (unaligned / aligneder + !!(unaligned % aligneder)) *
         aligneder;
}

typedef struct My_allocator My_allocator;
typedef const My_allocator *AllocatorV;
typedef void *(*const My_allocatorAlloc)(AllocatorV, size_t);
typedef void (*const My_allocatorFree)(AllocatorV, void *);
typedef void *(*const My_allocatorResize)(AllocatorV, void *, size_t);
typedef size_t (*const My_allocatorGetActualSize)(AllocatorV, void *);

typedef My_allocator *(*OwnAllocatorInit)(void);
typedef void (*OwnAllocatorDeInit)(My_allocator *);

typedef struct My_allocator {
  My_allocatorAlloc alloc;
  My_allocatorFree free;
  My_allocatorResize resize;
  My_allocatorGetActualSize size;  ///< optional, check every time
  char alignas(max_align_t) arb[]; ///< userdata
} My_allocator;

typedef struct {
  OwnAllocatorInit init;
  OwnAllocatorDeInit deInit;
} Own_Allocator;

typedef Own_Allocator OwnAllocator;

[[clang::ownership_returns(malloc), gnu::alloc_size(2)]]
void *aAlloc(AllocatorV allocator, size_t size);
[[gnu::alloc_size(3), clang::ownership_holds(aAlloc, 2)]]
void *aResize(AllocatorV allocator, void *oldptr, size_t size);
[[clang::ownership_takes(aAlloc, 2)]]
void aFree(AllocatorV allocator, void *oldptr);
#define aCreateHelper(allocator, type, count, ...) \
  ({type* res = ((type *)(aAlloc(allocator, sizeof(type) * count)));memset(res,0,sizeof(type)*count);   res; })
#define aCreate(allocator, type, ...) \
  aCreateHelper(allocator, type __VA_OPT__(, __VA_ARGS__), 1)

extern AllocatorV stdAlloc;

#endif // MY_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_ALLOCATOR_C (1)
#endif
#ifdef MY_ALLOCATOR_C
#include "assertMessage.h"
#include "mytypes.h"
#include <stdio.h>
[[clang::ownership_returns(malloc), gnu::alloc_size(2)]]
void *aAlloc(AllocatorV allocator, size_t size) {
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(size, "allocators cant allocate nothing");
#endif
  void *res = (allocator)->alloc(allocator, size);
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(res, "allocators cant return null");
  assertMessage(!((uintptr_t)res % alignof(max_align_t)), "allocator: %p pointer: %p", allocator, res);
#endif
  return res;
}
[[gnu::alloc_size(3), clang::ownership_holds(aAlloc, 2)]]
void *aResize(AllocatorV allocator, void *oldptr, size_t size) {
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(size, "allocators cant allocate nothing");
  assertMessage(oldptr, "allocators cant reallocate nothing");
#endif
  void *res = (allocator)->resize(allocator, oldptr, size);
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(res, "allocators cant return null");
  assertMessage(!((uintptr_t)res % alignof(max_align_t)), "allocator: %p pointer: %p", allocator, res);
#endif
  return res;
}
[[clang::ownership_takes(aAlloc, 2)]]
void aFree(AllocatorV allocator, void *oldptr) { ((allocator)->free(allocator, oldptr)); }
void *default_alloc(const My_allocator *allocator, size_t s) { return malloc(s); }
void *default_r_alloc(const My_allocator *allocator, void *p, size_t s) { return realloc(p, s); }
void default_free(const My_allocator *allocator, void *p) { return free(p); }
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
  #define DEFAULT_SIZE_GETTER (1)
  #include <malloc.h>
#elif defined(__APPLE__) || defined(__MACH__)
  #define DEFAULT_SIZE_GETTER (1)
  #include <malloc/malloc.h>
#else
  #undef DEFAULT_SIZE_GETTER
#endif

#ifdef DEFAULT_SIZE_GETTER
usize default_size(AllocatorV allocator, void *ptr) {
  #if defined(_WIN32) || defined(_WIN64)
  usize (*getSize)(void *) = _msize;
  #elif defined(__linux__)
  usize (*getSize)(void *) = malloc_usable_size;
  #elif defined(__APPLE__) || defined(__MACH__)
  usize (*getSize)(void *) = malloc_size;
  #else
      // compile error
  #endif
  return getSize(ptr);
}
#endif // DEFAULT_SIZE_GETTER
AllocatorV stdAlloc = (My_allocator[1]){{
    default_alloc,
    default_free,
    default_r_alloc,
#ifdef DEFAULT_SIZE_GETTER
    default_size,
#else
    nullptr
#endif
}};
#endif
