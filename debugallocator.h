#ifndef MY_DEBUG_ALLOCATOR_H
#define MY_DEBUG_ALLOCATOR_H
#include "allocator.h"
#include "assertMessage.h" // debug symbols
#include "hhmap.h"
#include "macros.h"
#include "mytypes.h"
#include "print.h"

struct dbgAlloc_config {
  AllocatorV allocator;
  FILE *log;
};
/**
 * `@param` **allocator**  allocator
 *      - backend allocator, it will also store itself here
 * `@return` debug allocator
 */
AllocatorV debugAllocatorInit(struct dbgAlloc_config config);
struct debugStats {
  usize max_memory, current_memory, total_calls, total_active_allocations;
};
struct debugStats debugAllocator_stats(AllocatorV allocator);
struct debugStats debugAllocator_clear(AllocatorV allocator);
#define PREPEND_DOT_MAC(...) .__VA_ARGS__,
#define debugAllocator(...) ({                     \
  struct dbgAlloc_config config = {                \
      APPLY_N(PREPEND_DOT_MAC, __VA_ARGS__)        \
  };                                               \
  config.allocator = config.allocator ?: stdAlloc; \
  debugAllocatorInit(config);                      \
})

/**
 * `@param` **allocator**  allocator
 *      - debug allocator
 * `@return` number of leaks found
 *      - will free itself along with any leaks it finds
 *      - will print traces to stdout
 */
int debugAllocatorDeInit(AllocatorV);

#if defined(MAKE_TEST_FN)
MAKE_TEST_FN(debug_allocator_test, {
  usize allocations = 5;
  usize total = 0;
  AllocatorV debug = debugAllocator(
      allocator = allocator,
  );
  for (each_RANGE(usize, i, 0, allocations)) {
    usize size = (i * i) + 1;
    int *ip = (int *)aAlloc(debug, 1);
    aResize(debug, ip, 1, size);
    total += aAlloc_align(size);
  };
  int n = debugAllocatorDeInit(debug);
  if (n != allocations) {
    return 2;
  }
  return 0;
});
#endif

#endif // MY_DEBUG_ALLOCATOR_H

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
#define MY_DEBUG_ALLOCATOR_C (1)
#endif

#if defined(MY_DEBUG_ALLOCATOR_C)
struct tracedata {
  char *fn;
  usize ln;
  usize size;
};
typedef struct {
  mHmap(void *, struct tracedata) map;
  AllocatorV actualAllocator;
  struct dbgAlloc_config config;
  usize max, current, total;
} debugAllocatorInternals;

typedef struct {
  My_allocator allocator[1];
  alignas(alignof(max_align_t)) debugAllocatorInternals internals[1];
} Debug_allocator_block;

void *debugAllocator_alloc(AllocatorV allocator, usize size, char *fn, usize ln);
void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size, char *fn, usize ln);
void debugAllocator_free(AllocatorV allocator, void *ptr, usize size, char *fn, usize ln);

struct debugStats debugAllocator_stats(AllocatorV allocator) {
  debugAllocatorInternals internals = *(debugAllocatorInternals *)allocator->arb;
  return (struct debugStats){
      .max_memory = internals.max,
      .current_memory = internals.current,
      .total_active_allocations = HMap_count((HMap *)internals.map),
      .total_calls = internals.total,
  };
}
REGISTER_SPECIAL_PRINTER_NEEDID(print_debug_stats, "dbga-stats", struct debugStats, {
  PUTS("{max storage: ");
  USETYPEPRINTER(usize, in.max_memory);
  PUTS(",");
  PUTS("current storage: ");
  USETYPEPRINTER(usize, in.current_memory);
  PUTS(",");
  PUTS("active allocations : ");
  USETYPEPRINTER(usize, in.total_active_allocations);
  PUTS(",");
  PUTS("total calls : ");
  USETYPEPRINTER(usize, in.total_calls);
  PUTS("}");
});
AllocatorV debugAllocatorInit(struct dbgAlloc_config config) {
  AllocatorV allocator = config.allocator;
  Debug_allocator_block *res = aCreate(allocator, Debug_allocator_block);
  res->internals[0] = (debugAllocatorInternals){
      .map = mHmap_init(allocator, void *, struct tracedata),
      .actualAllocator = allocator,
      .config = config,
      .max = 0,
      .current = 0,
      .total = 0,
  };
  My_allocator deffaultDebugAllocator = {
      .alloc = debugAllocator_alloc,
      .free = debugAllocator_free,
      .resize = debugAllocator_realloc,
      .size = allocator->size,
  };
  memcpy(res->allocator, &deffaultDebugAllocator, sizeof(My_allocator));
  return res->allocator;
}

int debugAllocatorDeInit(AllocatorV allocator) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  usize leaks = 0;

  pEsc r = (pEsc){.fg = {255, 0, 0}, .fgset = 1};
  pEsc g = (pEsc){.fg = {0, 255, 0}, .fgset = 1};
  pEsc b = (pEsc){.fg = {0, 0, 255}, .fgset = 1};
  pEsc rst = (pEsc){.reset = 1};

  FILE *out = internals->config.log;

  if (out) {
    print_wfO(fileprint, out, "maximum of {}{}{} bytes used at once\n", g, internals->max, rst);
    print_wfO(fileprint, out, "{}{}{} allocs / reallocs\n", r, internals->total, rst);
    print_wfO(fileprint, out, "leaked {} bytes\n", internals->current);
  }

  mHmap_foreach(
      internals->map,
      void *, ptr,
      struct tracedata, val,
      {
        leaks++;
        if (out) {
          print_wfO(
              fileprint, out,
              "leaked {}{usize}{} bytes at {}{ptr}{} in {cstr} at {}\n"
              "=========================================================\n",
              g, val.size, rst, b, ptr, r, val.fn, val.ln
          );
          print_wfO(
              fileprint, out,
              "from {cstr} line {}\n",
              val.fn, val.ln
          );

          print_wfO(
              fileprint, out,
              "=========================================================\n",
          );
          print_wfO(fileprint, out, "{}", rst);
        }
        (aFree)(realAllocator, (void *)ptr, val.size, val.fn, val.ln);
      }
  );
  mHmap_deinit(internals->map);
  aFree(realAllocator, (void *)allocator, sizeof(Debug_allocator_block));
  return leaks;
}

struct debugStats debugAllocator_clear(AllocatorV allocator) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  var_ res = debugAllocator_stats(allocator);
  mHmap_foreach(
      internals->map,
      void *, ptr,
      struct tracedata, val,
      {
        aFree(allocator, (void *)ptr, val.size);
      }
  );
  mHmap_clear(internals->map);
  return res;
}
void *debugAllocator_alloc(AllocatorV allocator, usize size, char *fn, usize ln) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  void *res = ((aAlloc)(realAllocator, size, fn, ln));
  internals->total++;

  var_ data =
      (struct tracedata){
          .size = size,
          .fn = fn,
          .ln = ln,
      };
  assertMessage(
      !mHmap_get(internals->map, res),
      "allocator allocated buisy memory"
  );
  mHmap_set(internals->map, res, data);
  internals->current += size;

  if (internals->current > internals->max)
    internals->max = internals->current;

  return res;
}

void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size, char *fn, usize ln) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  internals->total++;

  struct tracedata *i = mHmap_get(internals->map, ptr);
  assertMessage(i, "pointer not in allocator");

  internals->current -= i->size;
  assertMessage(mHmap_get(internals->map, ptr), "double free or corruption in : %s %zu", fn, ln);
  mHmap_rem(internals->map, ptr);

  void *res = (aResize)(realAllocator, ptr, i->size, size, fn, ln);
  assertMessage(
      !mHmap_get(internals->map, res),
      "allocator allocated buisy memory"
  );
  mHmap_set(
      internals->map,
      res,
      ((struct tracedata){
          .size = size,
          .fn = fn,
          .ln = ln,
      })
  );

  internals->current += size;

  if (internals->current > internals->max)
    internals->max = internals->current;
  return res;
}

void debugAllocator_free(AllocatorV allocator, void *ptr, usize size, char *fn, usize ln) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  (aFree)(realAllocator, ptr, size, fn, ln);
  struct tracedata *data = mHmap_get(internals->map, ptr);
  assertMessage(data, "pointer not in allocator");
  internals->current -= data->size;
  assertMessage(mHmap_get(internals->map, ptr), "double free or corruption in : %s %zu", fn, ln);
  mHmap_rem(internals->map, ptr);
}
#endif // MY_DEBUG_ALLOCATOR_C
