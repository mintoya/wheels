#include "assertMessage.h" // debug symbols
#include "hhmap.h"
#include "print.h"
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

My_allocator *debugAllocatorInit(AllocatorV allocator);
void debugAllocatorDeInit(My_allocator *allocator);

//
//
//
struct tracedata {
  void *trace[7];
  usize size;
};
typedef struct {
  mHmap(void *, struct tracedata) map;
  AllocatorV actualAllocator;
  usize max, current;
} debugAllocatorInternals;

void *debugAllocator_alloc(AllocatorV allocator, usize size);
void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size);
void debugAllocator_free(AllocatorV allocator, void *ptr);

usize debugAllocator_size(AllocatorV allocator, void *ptr);

debugAllocatorInternals *initializeInternals(AllocatorV allocator) {
  debugAllocatorInternals *internals = aCreate(allocator, debugAllocatorInternals);
  *internals = (debugAllocatorInternals){
      .actualAllocator = allocator,
      .map = mHmap_init(allocator, void *, struct tracedata),
      .current = 0,
      .max = 0,
  };
  return internals;
}
My_allocator *debugAllocatorInit(AllocatorV allocator) {
  My_allocator *res = aAlloc(allocator, sizeof(My_allocator));
  *res = (My_allocator){
      .alloc = debugAllocator_alloc,
      .ralloc = debugAllocator_realloc,
      .free = debugAllocator_free,
      .size = allocator->size,
      .arb = initializeInternals(allocator),
  };
  return res;
}
void debugAllocatorDeInit(My_allocator *allocator) {
  auto internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  mHmap_each(
      internals->map,
      void *, ptr,
      struct tracedata, val,
      {
        println("leaked {} bytes at {ptr}\n", val.size, ptr);
        char **names = backtrace_symbols(val.trace + 2, 5);
        for (auto i = 0; i < 5; i++)
          println("{cstr}", names[i]);
        println();

        free(names);
        aFree(realAllocator, ptr);
      }
  );
  mHmap_deinit(internals->map);
  aFree(realAllocator, internals);
  aFree(realAllocator, allocator);
}

void *debugAllocator_alloc(AllocatorV allocator, usize size) {
  auto internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  void *res = aAlloc(realAllocator, size);

  if (res) {
    struct tracedata data = {.size = size};
    backtrace(data.trace, 7);
    mHmap_set(internals->map, res, data);
    internals->current += size;
    if (internals->current > internals->max) {
      internals->max = internals->current;
    }
  }

  return res;
}

void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size) {
  auto internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;

  // Remove old tracking
  if (ptr) {
    struct tracedata *old = mHmap_get(internals->map, ptr);
    if (old) {
      internals->current -= old->size;
      mHmap_rem(internals->map, ptr);
    }
  }

  void *res = aRealloc(realAllocator, ptr, size);

  // Add new tracking
  if (res) {
    struct tracedata data = {.size = size};
    backtrace(data.trace, 7);
    mHmap_set(internals->map, res, data);
    internals->current += size;
    if (internals->current > internals->max) {
      internals->max = internals->current;
    }
  }

  return res;
}

void debugAllocator_free(AllocatorV allocator, void *ptr) {
  auto internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;

  if (ptr) {
    struct tracedata *data = mHmap_get(internals->map, ptr);
    if (data) {
      internals->current -= data->size;
      mHmap_rem(internals->map, ptr);
    }
  }

  aFree(realAllocator, ptr);
}
