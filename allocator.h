#ifndef MY_ALLOCATOR_H
#define MY_ALLOCATOR_H
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

[[gnu::const]]
uintptr_t lineup(size_t unaligned, size_t aligneder);

typedef struct My_allocator My_allocator;
typedef const My_allocator *AllocatorV;
typedef void *(*My_allocatorAlloc)(AllocatorV, size_t);
typedef void (*My_allocatorFree)(AllocatorV, void *);
typedef void *(*My_allocatorRealloc)(AllocatorV, void *, size_t);
typedef size_t (*My_allocatorGetActualSize)(AllocatorV, void *);

typedef My_allocator *(*OwnAllocatorInit)(void);
typedef void (*OwnAllocatorDeInit)(My_allocator *);

typedef struct My_allocator {
  My_allocatorAlloc alloc;
  My_allocatorFree free;
  My_allocatorRealloc ralloc;
  void *arb;                      ///< userdata
  My_allocatorGetActualSize size; ///< optional
} My_allocator;

typedef struct {
  OwnAllocatorInit init;
  OwnAllocatorDeInit deInit;
} Own_Allocator;

typedef Own_Allocator OwnAllocator;

[[gnu::alloc_size(2)]]
void *aAlloc(AllocatorV allocator, size_t size);
[[gnu::alloc_size(3)]]
void *aRealloc(AllocatorV allocator, void *oldptr, size_t size);
void aFree(AllocatorV allocator, void *oldptr);
#define aCreateHelper(allocator, type, count, ...) \
  ({type* res = ((type *)(aAlloc(allocator, sizeof(type) * count)));memset(res,0,sizeof(type)*count);   res; })
#define aCreate(allocator, type, ...) \
  aCreateHelper(allocator, type __VA_OPT__(, __VA_ARGS__), 1)

AllocatorV getDefaultAllocator(void);
#define defaultAlloc (getDefaultAllocator())

#endif // MY_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
// #define MY_ALLOCATOR_STRICTEST
#define MY_ALLOCATOR_C (1)
#endif
#ifdef MY_ALLOCATOR_C
#include "fptr.h"
#include <stdio.h>
void *aAlloc(AllocatorV allocator, size_t size) {
  void *res = (allocator)->alloc(allocator, size);
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(!((uintptr_t)res % alignof(max_align_t)), "allocator: %p pointer: %p", allocator, res);
#endif
  return res;
}
void *aRealloc(AllocatorV allocator, void *oldptr, size_t size) {
  void *res = (allocator)->ralloc(allocator, oldptr, size);
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(!((uintptr_t)res % alignof(max_align_t)), "allocator: %p pointer: %p", allocator, res);
#endif
  return res;
}
void aFree(AllocatorV allocator, void *oldptr) {
  return ((allocator)->free(allocator, oldptr));
}
void *default_alloc(const My_allocator *allocator, size_t s) {
  return malloc(s);
}
void *default_r_alloc(const My_allocator *allocator, void *p, size_t s) {
  return realloc(p, s);
}
void default_free(const My_allocator *allocator, void *p) {
  return free(p);
}
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
  #define DEFAULT_SIZE_GETTER (1)
  #include <malloc.h>
#elif defined(__APPLE__) || defined(__MACH__)
  #define DEFAULT_SIZE_GETTER (1)
  #include <malloc/malloc.h>
#else
  #undef DEFAULT_SIZE_GETTER
#endif

#include "assertMessage.h"
usize default_size(AllocatorV allocator, void *ptr) {
  usize (*getSize)(void *) = NULL;
#if defined(_WIN32) || defined(_WIN64)
  getSize = _msize;
#elif defined(__linux__)
  getSize = malloc_usable_size;
#elif defined(__APPLE__) || defined(__MACH__)
  getSize = malloc_size;
#else
  getSize = NULL;
#endif
  assertMessage(getSize, "called getsize on unsupported platform");
  return getSize(ptr);
}
AllocatorV getDefaultAllocator(void) {
  static const My_allocator defaultAllocator = (My_allocator){
      default_alloc,
      default_free,
      default_r_alloc,
      NULL,
#ifdef DEFAULT_SIZE_GETTER
      default_size
#else
      NULL
#endif
  };
  // printf("default allocator: %p\n", &defaultAllocator);
  return &defaultAllocator;
}
[[gnu::const]]
uintptr_t lineup(uptr unaligned, size_t aligneder) {
  if (unaligned % aligneder != 0) {
    return unaligned + aligneder - unaligned % aligneder;
  } else {
    return unaligned;
  }
}

#endif
