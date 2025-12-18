#ifndef MY_ALLOCATOR_H
#define MY_ALLOCATOR_H
#include <stdint.h>
#include <stddef.h>

[[gnu::pure]]
static uintptr_t lineup(size_t unaligned, size_t aligneder) {
  if (unaligned % aligneder != 0) {
    return unaligned + aligneder - unaligned % aligneder;
  }
  return unaligned;
}
typedef struct My_allocator My_allocator;
typedef void *(*My_allocatorAlloc)(const My_allocator *, size_t);
typedef void (*My_allocatorFree)(const My_allocator *, void *);
typedef void *(*My_allocatorRealloc)(const My_allocator *, void *, size_t);
typedef size_t (*My_allocatorGetActualSize)(void *);

typedef My_allocator *(*OwnAllocatorInit)(void);
typedef void (*OwnAllocatorDeInit)(My_allocator *);

typedef struct My_allocator {
  My_allocatorAlloc alloc;
  My_allocatorFree free;
  My_allocatorRealloc ralloc;
  void *arb; // state
  My_allocatorGetActualSize size;
} My_allocator;

typedef struct {
  OwnAllocatorInit init;
  OwnAllocatorDeInit deInit;
} Own_Allocator;

typedef My_allocator MyAllocator;
typedef Own_Allocator OwnAllocator;

void *default_alloc(const My_allocator *allocator, size_t s);
void *default_r_alloc(const My_allocator *allocator, void *p, size_t s);
void default_free(const My_allocator *allocator, void *p);
// extern const My_allocator *defaultAlloc;
extern inline const My_allocator *getDefaultAllocator(void);
#define defaultAlloc (getDefaultAllocator())

// not making one for getsize
#define aAlloc(allocator, size) ((allocator)->alloc(allocator, size))
#define aRealloc(allocator, oldptr, size) ((allocator)->ralloc(allocator, oldptr, size))
#define aFree(allocator, oldptr) ((allocator)->free(allocator, oldptr))
#endif // MY_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_ALLOCATOR_C (1)
#endif
#ifdef MY_ALLOCATOR_C
#include <stdlib.h>
void *default_alloc(const My_allocator *allocator, size_t s) {
  (void)allocator;
  return malloc(s);
}
#include "fptr.h"
void *default_r_alloc(const My_allocator *allocator, void *p, size_t s) {
  (void)allocator;
  assertMessage(p);
  void *res = realloc(p, s);
  assertMessage(res);
  return res;
}
void default_free(const My_allocator *allocator, void *p) {
  (void)allocator;
  return free(p);
}
#include "fptr.h"
const My_allocator defaultAllocator = (My_allocator){default_alloc, default_free, default_r_alloc};
const My_allocator *getDefaultAllocator(void) {
  return &defaultAllocator;
}

#endif
