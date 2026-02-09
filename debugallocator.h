#ifndef MY_DEBUG_ALLOCATOR_H
#define MY_DEBUG_ALLOCATOR_H
#include "assertMessage.h" // debug symbols
#include "hhmap.h"
#include "print.h"
#include "types.h"
#include <stdlib.h>

struct dbgAlloc_config {
  bool track_trace;
  bool track_total;
  FILE *log;
};
/**
 * `@param` **allocator**  allocator
 *      - backend allocator, it will also store itself here
 * `@return` debug allocator
 */
My_allocator *debugAllocatorInit(AllocatorV allocator, struct dbgAlloc_config config);
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
  struct dbgAlloc_config config;
  usize max, current, total;
} debugAllocatorInternals;

void *debugAllocator_alloc(AllocatorV allocator, usize size);
void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size);
void debugAllocator_free(AllocatorV allocator, void *ptr);

usize debugAllocator_size(AllocatorV allocator, void *ptr);

My_allocator *debugAllocatorInit(AllocatorV allocator, struct dbgAlloc_config config) {
  My_allocator *res = aCreate(allocator, My_allocator);
  debugAllocatorInternals *internals = aCreate(allocator, typeof(*internals));
  *internals = (debugAllocatorInternals){
      .map = mHmap_init(allocator, void *, struct tracedata),
      .actualAllocator = allocator,
      .config = config,
      .max = 0,
      .current = 0,
      .total = 0,
  };
  *res = (My_allocator){
      .alloc = debugAllocator_alloc,
      .free = debugAllocator_free,
      .ralloc = debugAllocator_realloc,
      .arb = internals,
      .size = allocator->size,
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

  const bool track_trace = internals->config.track_trace;
  const bool track_total = internals->config.track_total;
  FILE *out = internals->config.log;

  if (out) {
    if (track_total) {
      print_wfO(fileprint, out, "maximum of {}{}{} bytes used at once\n", g, internals->max, rst);
      print_wfO(fileprint, out, "{}{}{} allocs / reallocs\n", r, internals->total, rst);
      print_wfO(fileprint, out, "leaked {} bytes\n", internals->current);
    }
  }

  mHmap_foreach(
      internals->map,
      void *, ptr,
      struct tracedata, val,
      {
        leaks++;
        if (track_trace && out) {
          print_wfO(
              fileprint, out,
              "leaked {}{usize}{} bytes at {}{ptr}{}\n"
              "=========================================================\n",
              g, val.size, rst, b, ptr, r
          );

          char **names = backtrace_symbols(val.trace + 2, 5);
          for (usize i = 0; i < 5; i++)
            print_wfO(fileprint, out, "{cstr}\n", names[i]);
          print_wfO(
              fileprint, out,
              "=========================================================\n",
          );
          print_wfO(fileprint, out, "{}", rst);
          free(names);
        }
        aFree(realAllocator, (void *)ptr);
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
  internals->total++;

  struct tracedata data;
  if (internals->config.track_trace)
    backtrace(data.trace, 7);
  data.size = size;
  mHmap_set(internals->map, res, data);
  if (internals->config.track_total)
    internals->current += size;

  if (internals->current > internals->max)
    internals->max = internals->current;

  return res;
}

void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  internals->total++;

  struct tracedata *i = mHmap_get(internals->map, ptr);

  if (i)
    internals->current -= i->size;
  mHmap_rem(internals->map, ptr);

  void *res = aRealloc(realAllocator, ptr, size);

  struct tracedata data = {.size = size};
  if (internals->config.track_trace)
    backtrace(data.trace, 7);
  mHmap_set(internals->map, res, data);

  if (internals->config.track_total)
    internals->current += size;

  if (internals->current > internals->max)
    internals->max = internals->current;

  return res;
}

void debugAllocator_free(AllocatorV allocator, void *ptr) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;

  aFree(realAllocator, ptr);

  struct tracedata *data = mHmap_get(internals->map, ptr);
  // internals->current -= data ? data->size : 0;
  if (internals->config.track_total)
    internals->current -= data->size;
  mHmap_rem(internals->map, ptr);
}
#endif // MY_DEBUG_ALLOCATOR_C
