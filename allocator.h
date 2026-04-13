#ifndef MY_ALLOCATOR_H
#define MY_ALLOCATOR_H
#include "mytypes.h"
#include <stdalign.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#if defined(__cplusplus)
  #include <cstddef>
#else
  #include <stddef.h>
#endif
#include <stdint.h>

#define MY_ALLOCATOR_STRICTEST

__attribute__((const)) static inline uptr lineup(uptr unalinged, usize a) { return (unalinged + (a - 1)) & ~(a - 1); }
__attribute__((const)) static inline uptr aAlloc_align(uptr unaligned) {
  return lineup(
      unaligned, alignof(max_align_t) > sizeof(usize)
                     ? alignof(max_align_t)
                     : sizeof(usize)
  );
}
// pretty sure these are always the same?

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
typedef void *(*const My_allocatorAlloc)(AllocatorV, size_t, char *, usize);
/**
 *  equivalent of free
 *  @param 1 allocator
 *  @param 2 pointer    *non-null* pointer must've come from same allocator
 */
typedef void (*const My_allocatorFree)(AllocatorV, void *, usize, char *, usize);
/**
 *  equivalent of realloc
 *    intended to behave as if
 *      same requirements as My_allocatorAlloc and My_allocatorFree
 *  @param 1 allocator
 *  @param 2 pointer    *non-null* pointer must've come from same allocator
 *  @param 3 size       new size of allocation, non-zero
 *  @return moved pointer
 */
typedef void *(*const My_allocatorResize)(AllocatorV, void *, size_t, char *, usize);
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
  My_allocatorResize /*   */ resize; ///< optional
  My_allocatorGetUsable /**/ size;   ///< optional
  max_align_t /*          */ arb[];
} My_allocator;

#define aAlloc(...) ((aAlloc)(__VA_ARGS__, __FILE__, __LINE__))
#define aResize(...) ((aResize)(__VA_ARGS__, __FILE__, __LINE__))
#define aFree(...) ((aFree)(__VA_ARGS__, __FILE__, __LINE__))
void *(aAlloc)(AllocatorV allocator, size_t size, char *, usize);
void *(aResize)(AllocatorV allocator, void *oldptr, size_t oldsize, size_t newsize, char *, usize);
void(aFree)(AllocatorV allocator, void *oldptr, usize size, char *file, usize line);

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
void *(aAlloc)(AllocatorV allocator, size_t size, char *file, usize line) {
#ifdef MY_ALLOCATOR_STRICTEST
  size = aAlloc_align(size);
  assertMessage(size, "allocators cant allocate nothing : %s,%zu", file, line);
#endif
  void *res = (allocator)->alloc(allocator, size, file, line);
#ifdef MY_ALLOCATOR_STRICTEST
  assertMessage(res, "allocators cant return null");
  assertMessage(!((uintptr_t)res % alignof(max_align_t)), "wrong alignment out of allocator");
#endif
  return res;
}
void *(aResize)(AllocatorV allocator, void *oldptr, usize oldsize, size_t newsize, char *file, usize line) {
#ifdef MY_ALLOCATOR_STRICTEST
  oldsize = aAlloc_align(oldsize);
  newsize = aAlloc_align(newsize);
  assertMessage(newsize, "allocators cant allocate nothing : %s,%zu", file, line);
  assertMessage(oldptr, "allocators cant reallocate nothing");
#endif
  void *res;
  if (allocator->resize) {

    void *result = (allocator)->resize(allocator, oldptr, newsize, file, line);
#ifdef MY_ALLOCATOR_STRICTEST
    assertMessage(result, "allocators cant return null, r");
    assertMessage(!((uintptr_t)result % alignof(max_align_t)), "wrong alignment out of allocator, r");
#endif
    res = result;
  } else {

    max_align_t *result = (max_align_t *)allocator->alloc(allocator, newsize, file, line);
#ifdef MY_ALLOCATOR_STRICTEST
    assertMessage(result, "allocators cant return null, r");
    assertMessage(!((uintptr_t)result % alignof(max_align_t)), "wrong alignment out of allocator, r");
#endif
    usize size = oldsize < newsize ? oldsize : newsize;
    __builtin_memcpy(result, oldptr, size);
    allocator->free(allocator, oldptr, oldsize, file, line);
    res = (void *)result;
  }
  return res;
}
void(aFree)(AllocatorV allocator, void *oldptr, usize size, char *file, usize line) {
#ifdef MY_ALLOCATOR_STRICTEST
  size = aAlloc_align(size);
  assertMessage(oldptr, "allocators dont deal with null: %s %zu", file, line);
#endif
  (allocator)->free(allocator, oldptr, size, file, line);
}
void *default_alloc(const My_allocator *allocator, size_t s, char *, usize) { return malloc(aAlloc_align(s)); }
void *default_r_alloc(const My_allocator *allocator, void *p, size_t s, char *, usize) { return realloc(p, s); }
void default_free(const My_allocator *allocator, void *p, usize size, char *, usize) { return free(p); }
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
    nullptr,
// default_r_alloc,
#ifdef DEFAULT_SIZE_GETTER
    default_size,
#else
    nullptr
#endif
}};
#endif
