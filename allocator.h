#ifndef MY_ALLOCATOR_H
#define MY_ALLOCATOR_H
#include "mytypes.h"

#define MY_ALLOCATOR_STRICTEST

// #define lineup(u, a) ({var_ _a = a ; var_ _u = u;( ((_u + (_a - 1)) / _a) * _a ); })
__attribute__((const)) static inline uptr lineup(uptr u, usize a) {
  return (((u + (a - 1)) / a) * a);
}
// an allocated pointer should be uncheaged when passed trought this function
__attribute__((const)) static inline uptr aAlloc_align(uptr unaligned) {
  return lineup(
      unaligned, MAX$(alignof(myAlign), sizeof(usize))
  );
}

//
// types
//

typedef struct My_allocator My_allocator;
typedef const My_allocator *AllocatorV;
/**
 *  equivalent of malloc
 *    cannot return null
 *    cannot allocate 0
 *    allocation always aligned to myAlign
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
typedef void *(*const My_allocatorResize)(AllocatorV, void *, size_t, size_t, char *, usize);
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
  // TODO  clearall?
  myAlign /*          */ arb[];
} My_allocator;

//
// helpers
//

#define aAlloc(...) ((aAlloc)(__VA_ARGS__, (char *)__FUNCTION__, __LINE__))
#define aResize(...) ((aResize)(__VA_ARGS__, (char *)__FUNCTION__, __LINE__))
#define aFree(...) ((aFree)(__VA_ARGS__, (char *)__FUNCTION__, __LINE__))
void *(aAlloc)(AllocatorV allocator, size_t size, char *, usize);
void *(aResize)(AllocatorV allocator, void *oldptr, size_t oldsize, size_t newsize, char *, usize);
void(aFree)(AllocatorV allocator, void *oldptr, usize size, char *file, usize line);

#include "macros.h"
#include <string.h>
#define aCreate(allocator, type, ...)                                      \
  /* optional count argument, defaults to 1*/                              \
  DIAGNOSTIC_PUSH("-Weverything")                                          \
  *(type(*)[(__VA_OPT__(1) + 0) ? __VA_ARGS__ + 0 : 1]) DIAGNOSTIC_POP()({ \
    size_t _count = VA_SWITCH(1, __VA_ARGS__);                             \
    type *_res = ((type *)(aAlloc(allocator, sizeof(type) * _count)));     \
    memset(_res, 0, sizeof(type) * _count);                                \
    _res;                                                                  \
  })
#define aValue(allocator, value) ({              \
  var_ _rse = aCreate(allocator, typeof(value)); \
  _rse[0] = value;                               \
  _rse;                                          \
})
// #define aDestroy(allocator , value )
#if defined(__cplusplus)
#endif

extern AllocatorV stdAlloc;

#endif // MY_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_ALLOCATOR_C (1)
#endif
#if defined(MY_ALLOCATOR_C)
#include "assertMessage.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef MY_ALLOCATOR_STRICTEST
static inline void check_size(size_t size, char *file, usize line) {
  assertMessage(size, "allocators cant allocate nothing : %s,%zu", file, line);
}

static inline void check_realloc_ptr(void *oldptr, char *file, usize line) {
  assertMessage(oldptr, "tried to reallocate a null pointer : %s,%zu", file, line);
}

static inline void check_free_ptr(void *oldptr, char *file, usize line) {
  assertMessage(oldptr, "tried to free a null pointer", file, line);
}

static inline void check_result(AllocatorV allocator, usize request_size, void *res) {
  if (!res || ((uptr)res % alignof(myAlign))) {
    fprintf(
        stdout,
        "requested : %zu\n"
        "allocator:\n"
        "\ta:%p\n"
        "\tf:%p\n"
        "\tr:%p\n"
        "\ts:%p\n",
        request_size,
        allocator->alloc,
        allocator->free,
        allocator->resize,
        allocator->size
    );
    assertMessage(res, "allocators cant return null");
    assertMessage(false, "wrong alignment out of allocator %zu", (uptr)res);
  }
}
#endif

void *(aAlloc)(AllocatorV allocator, size_t size, char *file, usize line) {
#ifdef MY_ALLOCATOR_STRICTEST
  size = aAlloc_align(size);
  check_size(size, file, line);
#endif
  void *res = (allocator)->alloc(allocator, size, file, line);
#ifdef MY_ALLOCATOR_STRICTEST
  check_result(allocator, size, res);
#endif
  return res;
}
void *(aResize)(AllocatorV allocator, void *oldptr, usize oldsize, size_t newsize, char *file, usize line) {
#ifdef MY_ALLOCATOR_STRICTEST
  oldsize = aAlloc_align(oldsize);
  newsize = aAlloc_align(newsize);
  check_size(newsize, file, line);
  check_realloc_ptr(oldptr, file, line);
#endif
  void *res;
  if (allocator->resize) {

    void *result = (allocator)->resize(allocator, oldptr, oldsize, newsize, file, line);
#ifdef MY_ALLOCATOR_STRICTEST
    check_result(allocator, newsize, result);
#endif
    res = result;
  } else {

    void *result = aAlloc(allocator, newsize);
    usize size = oldsize < newsize ? oldsize : newsize;
    memcpy(result, oldptr, size);
    allocator->free(allocator, oldptr, oldsize, file, line);
    res = (void *)result;
  }
  return res;
}
void(aFree)(AllocatorV allocator, void *oldptr, usize size, char *file, usize line) {
#ifdef MY_ALLOCATOR_STRICTEST
  size = aAlloc_align(size);
  check_free_ptr(oldptr, file, line);
#endif
  (allocator)->free(allocator, oldptr, size, file, line);
}

void *default_alloc(const My_allocator *allocator, size_t s, char *, usize) {
  var_ res = malloc(aAlloc_align(s));
  assertMessage(!((uptr)res % alignof(myAlign)), "%i", (uptr)res % alignof(myAlign));
  return res;
}
void *default_r_alloc(const My_allocator *allocator, void *p, size_t os, size_t ns, char *, usize) { return realloc(p, ns); }
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
    default_r_alloc,
#ifdef DEFAULT_SIZE_GETTER
    default_size,
#else
    nullptr
#endif
}};
#endif
