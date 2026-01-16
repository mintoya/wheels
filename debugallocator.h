#ifndef MY_DEBUG_ALLOCATOR_H
#define MY_DEBUG_ALLOCATOR_H
#include "assertMessage.h" // debug symbols
#include "hhmap.h"
#include "print.h"
#include "types.h"
#include <stdlib.h>

/**
 * `@param` **allocator**  allocator
 *      - backend allocator, it will also store itself here
 * `@return` debug allocator
 */
My_allocator *debugAllocatorInit(AllocatorV allocator);
/**
 * `@param` **allocator**  allocator
 *      - debug allocator
 * `@return` number of leaks found
 *      - will free itself along with any leaks it finds
 *      - will print traces to stdout
 */
int debugAllocatorDeInit(My_allocator *allocator);

#endif // MY_DEBUG_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define MY_DEBUG_ALLOCATOR_C (1)
#endif
#ifdef MY_DEBUG_ALLOCATOR_C
struct tracedata {
  void *trace[7];
  usize size;
};
typedef struct {
  mHmap(void *, struct tracedata) map;
  AllocatorV actualAllocator;
  usize max, current, totalAllocations;
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
      .totalAllocations = 0,
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
int debugAllocatorDeInit(My_allocator *allocator) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  usize leaks = 0;

  pEsc r = (pEsc){.fg = {255, 0, 0}, .fgset = 1};
  pEsc g = (pEsc){.fg = {0, 255, 0}, .fgset = 1};
  pEsc b = (pEsc){.fg = {0, 0, 255}, .fgset = 1};
  pEsc rst = (pEsc){.reset = 1};

  println("maximum of {}{}{} bytes used at once", g, internals->max, rst);
  println("{}{}{} allocs / reallocs", r, internals->totalAllocations, rst);
  println("leaked {} bytes", internals->current);

  mHmap_each(
      internals->map,
      void *, ptr,
      struct tracedata, val,
      {
        leaks++;
        println(
            "leaked {}{usize}{} bytes at {}{ptr}{}\n"
            "=========================================================\n",
            g, val.size, rst, b, ptr, r
        );
        char **names = backtrace_symbols(val.trace + 2, 5);
        for (usize i = 0; i < 5; i++)
          println("{cstr}", names[i]);
        println(
            "=========================================================\n",
        );
        println("{}", rst);

        free(names);
        aFree(realAllocator, ptr);
      }
  );
  mHmap_deinit(internals->map);
  aFree(realAllocator, internals);
  aFree(realAllocator, allocator);
  return leaks;
}

void *debugAllocator_alloc(AllocatorV allocator, usize size) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  void *res = aAlloc(realAllocator, size);
  internals->totalAllocations++;

  struct tracedata data;
  backtrace(data.trace, 7);
  data.size = size;
  mHmap_set(internals->map, res, data);
  internals->current += size;

  if (internals->current > internals->max)
    internals->max = internals->current;

  return res;
}

void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  internals->totalAllocations++;

  struct tracedata *i = mHmap_get(internals->map, ptr);

  if (i)
    internals->current -= i->size;
  mHmap_rem(internals->map, ptr);

  void *res = aRealloc(realAllocator, ptr, size);

  struct tracedata data = {.size = size};
  backtrace(data.trace, 7);
  mHmap_set(internals->map, res, data);
  internals->current += size;
  if (internals->current > internals->max)
    internals->max = internals->current;

  return res;
}

void debugAllocator_free(AllocatorV allocator, void *ptr) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;

  struct tracedata *data = mHmap_get(internals->map, ptr);

  internals->current -= data->size;
  // internals->current -= data ? data->size : 0;

  mHmap_rem(internals->map, ptr);

  aFree(realAllocator, ptr);
}
#endif // MY_DEBUG_ALLOCATOR_C
