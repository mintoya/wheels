#ifndef MY_ALLOCATOR_H
#define MY_ALLOCATOR_H
#include <stdalign.h>
#if defined(__cplusplus)
  #include <cstddef>
#else
  #include <stddef.h>
#endif
#include <stdint.h>

#define MY_ALLOCATOR_STRICTEST

__attribute__((const)) extern inline uintptr_t lineup(uintptr_t unaligned, size_t aligneder) {
  return (unaligned / aligneder + !!(unaligned % aligneder)) *
         aligneder;
}

typedef struct My_allocator My_allocator;
typedef const My_allocator *AllocatorV;
/**
 *  equivalent of malloc
 *    cannot return null
 *    cannot allocate 0
 *    allocation always aligned to max_align_t
 *  @param 1 allocator
 *  @param 2 size    *non-0*
 */
typedef void *(*const My_allocatorAlloc)(AllocatorV, size_t);
/**
 *  equivalent of free
 *  @param 1 allocator
 *  @param 2 pointer    *non-null* pointer must've come from same allocator
 */
typedef void (*const My_allocatorFree)(AllocatorV, void *);
/**
 *  equivalent of realloc
 *    intended to behave as if
 *      same requirements as My_allocatorAlloc and My_allocatorFree
 *  @param 1 allocator
 *  @param 2 pointer    *non-null* pointer must've come from same allocator
 *  @param 3 size       new size of allocation, non-zero
 *  @return moved pointer
 */
typedef void *(*const My_allocatorResize)(AllocatorV, void *, size_t);
/**
 *  get the size of an allocation/reallocation
 *  @param 1 allocator
 *  @param 2 pointer    *non-null* pointer must've come from same allocator
 *  @return uszble allocation size
 */
typedef size_t (*const My_allocatorGetUsable)(AllocatorV, void *);

typedef struct My_allocator {
  My_allocatorAlloc /*    */ alloc;
  My_allocatorFree /*     */ free;
  My_allocatorResize /*   */ resize;
  My_allocatorGetUsable /**/ size; ///< optional
  max_align_t /*          */ arb[];
} My_allocator;

void *aAlloc(AllocatorV allocator, size_t size);
void *aResize(AllocatorV allocator, void *oldptr, size_t size);
void aFree(AllocatorV allocator, void *oldptr);

#include "macros.h"
#define aCreate(allocator, type, ...)                                      \
  /* optional count argument, defaults to 1*/                              \
  DIAGNOSTIC_PUSH("-Weverything")                                          \
  *(type(*)[(__VA_OPT__(1) + 0) ? __VA_ARGS__ + 0 : 1]) DIAGNOSTIC_POP()({ \
    size_t _count = (__VA_OPT__(1) + 0) ? __VA_ARGS__ + 0 : 1;             \
    type *_res = ((type *)(aAlloc(allocator, sizeof(type) * _count)));     \
    __builtin_memset(_res, 0, sizeof(type) * _count);                      \
    _res;                                                                  \
  })

extern AllocatorV stdAlloc;

#endif // MY_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_ALLOCATOR_C (1)
#endif
#if defined(MY_ALLOCATOR_C)
#include "assertMessage.h"
#include "mytypes.h"
#include <stdio.h>
void *aAlloc(AllocatorV allocator, size_t size) {
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(size, "allocators cant allocate nothing");
#endif
  void *res = (allocator)->alloc(allocator, size);
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(res, "allocators cant return null");
  assertMessage(!((uintptr_t)res % alignof(max_align_t)), "wrong alignment out of allocator");
#endif
  return res;
}
void *aResize(AllocatorV allocator, void *oldptr, size_t size) {
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(size, "allocators cant allocate nothing");
  assertMessage(oldptr, "allocators cant reallocate nothing");
#endif
  void *res = (allocator)->resize(allocator, oldptr, size);
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(res, "allocators cant return null, r");
  assertMessage(!((uintptr_t)res % alignof(max_align_t)), "wrong alignment out of allocator, r");
#endif
  return res;
}
void aFree(AllocatorV allocator, void *oldptr) {

#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(oldptr, "allocators dont deal with null");
#endif
  ((allocator)->free(allocator, oldptr));
}
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
AllocatorV stdAlloc = (typeof(stdAlloc))(My_allocator[1]){{
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
