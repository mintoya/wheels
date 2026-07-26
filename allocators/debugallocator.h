#if !defined MY_DEBUG_ALLOCATOR_H
  #define MY_DEBUG_ALLOCATOR_H
  #include "../allocator.h"
  #include "../hxmap.h"
  #include "../print/print_pre.h"
  #include <stdio.h>

struct tracedata {
  char *fn;
  usize ln;
  usize size;
};
typedef struct {
  usize insize, outsize;
  void *iptr, *optr;
  struct tracedata trace;
} allocationType;
struct dbgAlloc_config {
  AllocatorV allocator;
  FILE *log;
  fnptrof((allocationType *), void) on_call;
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
  #define debugAllocator(...) ({                     \
    struct dbgAlloc_config config = {                \
        __VA_ARGS__                                  \
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

#endif // MY_DEBUG_ALLOCATOR_H

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define MY_DEBUG_ALLOCATOR_C (1)
#endif

#if defined(MY_DEBUG_ALLOCATOR_C)

  #include "../macros.h"
  #include "../mytypes.h"
  #include "../print/print_pre.h"

typedef struct {
  mxmap(void *, struct tracedata) map;
  AllocatorV actualAllocator;
  struct dbgAlloc_config config;
  usize max, current, total;
} debugAllocatorInternals;

typedef struct {
  My_allocator allocator[1];
  debugAllocatorInternals internals[1];
} Debug_allocator_block;

  #include "../tests.h"
test_fn(debug_allocator_test) {
  usize allocations = 10;
  AllocatorV debug = debugAllocator(
      allocator = allocator,
  );
  foreach (var_ i, range(0, allocations)) {
    usize size = (i * i) + 1;
    int *ip = (int *)aAlloc(debug, 1);
    aResize(debug, ip, 1, size);
  };

  debugAllocatorInternals *internals = ((debugAllocatorInternals *)debug->arb);
  int n1 = ((hxmap *)internals->map)->count;
  int n2 = 0;
  foreach (var_ it, mxmap_iter(internals->map, void *, struct tracedata))
    n2++;
  int n = debugAllocatorDeInit(debug);
  test_assert(n == allocations && n1 == allocations && n2 == n1);
}

void *debugAllocator_alloc(AllocatorV allocator, usize size, char *fn, usize ln);
void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize oldsize, usize newsize, char *fn, usize ln);
void debugAllocator_free(AllocatorV allocator, void *ptr, usize size, char *fn, usize ln);

struct debugStats debugAllocator_stats(AllocatorV allocator) {
  debugAllocatorInternals internals = *(debugAllocatorInternals *)allocator->arb;
  return (struct debugStats){
      .max_memory = internals.max,
      .current_memory = internals.current,
      .total_active_allocations = ((hxmap *)internals.map)->count,
      .total_calls = internals.total,
  };
}
typePrinter("dbga-stats", struct debugStats) {
  PUTS("{max storage: ");
  USENAMEDPRINTER("usize", in.max_memory);
  PUTS(",");
  PUTS("current storage: ");
  USENAMEDPRINTER("usize", in.current_memory);
  PUTS(",");
  PUTS("active allocations : ");
  USENAMEDPRINTER("usize", in.total_active_allocations);
  PUTS(",");
  PUTS("total calls : ");
  USENAMEDPRINTER("usize", in.total_calls);
  PUTS("}");
}
AllocatorV debugAllocatorInit(struct dbgAlloc_config config) {
  AllocatorV allocator = config.allocator;
  Debug_allocator_block *res = aCreate(allocator, Debug_allocator_block);

  res->internals[0] = (debugAllocatorInternals){
      .map = mxmap_init(allocator, void *, struct tracedata),
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

  const pEsc r = (pEsc){.fg = {-1, 0, 0}, .fgset = true};
  const pEsc g = (pEsc){.fg = {0, -1, 0}, .fgset = true};
  const pEsc b = (pEsc){.fg = {0, 0, -1}, .fgset = true};
  const pEsc rst = (pEsc){.reset = true};

  FILE *out = internals->config.log;

  if (out) {
    print_wfO(fileprint, out, "maximum of {}{}{} bytes used at once\n", g, internals->max, rst);
    print_wfO(fileprint, out, "{}{}{} allocs / reallocs\n", r, internals->total, rst);
    print_wfO(fileprint, out, "leaked {} bytes\n", internals->current);
  }

  foreach (var_ kv, hxmap_iter(internals->map)) {
    leaks++;
    var_ key = *(void **)kv.key;
    var_ val = *(mxmap_valType(internals->map) *)kv.val;
    if (out) {
      print_wfO(
          fileprint, out, "leaked {}{usize}{} bytes at {}{ptr}{} in {cstr} at {}\n"
                          "=========================================================\n",
          g,
          val.size,
          rst,
          b,
          key,
          r,
          val.fn,
          val.ln
      );
      print_wfO(
          fileprint, out, "from {cstr} line {}\n", val.fn, val.ln
      );

      print_wfO(
          fileprint, out, "=========================================================\n",
      );
      print_wfO(fileprint, out, "{}", rst);
    }
    (aFree)(realAllocator, (void *)key, val.size, val.fn, val.ln);
  }
  mxmap_deinit(internals->map);
  aFree(realAllocator, (void *)allocator, sizeof(Debug_allocator_block));
  return leaks;
}

struct debugStats debugAllocator_clear(AllocatorV allocator) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  var_ res = debugAllocator_stats(allocator);
  foreach (var_ kv, hxmap_iter(internals->map)) {
    var_ key = *(void **)kv.key;
    var_ val = *(mxmap_valType(internals->map) *)kv.val;
    aFree(internals->actualAllocator, (void *)key, val.size);
    mxmap_rem(internals->map, key);
  }
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
      !mxmap_get(internals->map, res),
      "allocator allocated buisy memory"
  );
  mxmap_set(internals->map, res, data);
  internals->current += size;

  if (internals->current > internals->max)
    internals->max = internals->current;

  if (internals->config.on_call) {
    allocationType t;
    t.insize = 0;
    t.iptr = nullptr;
    t.optr = res;
    t.outsize = size;
    t.trace = data;
    internals->config.on_call(&t);
  }

  return res;
}

void debugAllocator_free(AllocatorV allocator, void *ptr, usize size, char *fn, usize ln) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  struct tracedata *data = mxmap_get(internals->map, ptr);
  assertMessage(data, "pointer not in allocator , from %lu %s", ln, fn);
  struct tracedata datak = *data;
  internals->current -= data->size;
  (aFree)(realAllocator, ptr, data->size, fn, ln);

  if (internals->config.on_call) {
    allocationType t;
    t.insize = size;
    t.iptr = ptr;
    t.optr = nullptr;
    t.outsize = 0;
    t.trace = datak;
    internals->config.on_call(&t);
  }
  mxmap_rem(internals->map, ptr);
}
void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize oldsize, usize newsize, char *fn, usize ln) {
  debugAllocatorInternals *internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  struct tracedata *data = mxmap_get(internals->map, ptr);
  assertMessage(data, "pointer not in allocator , from %lu %s", ln, fn);
  internals->current -= data->size;
  mxmap_rem(internals->map, ptr);
  void *res = ((aResize)(realAllocator, ptr, oldsize, newsize, fn, ln));
  internals->total++;
  assertMessage(
      !mxmap_get(internals->map, res),
      "allocator allocated buisy memory"
  );
  var_ da =
      (struct tracedata){
          .size = newsize,
          .fn = fn,
          .ln = ln,
      };
  mxmap_set(internals->map, res, da);

  internals->current += newsize;

  if (internals->current > internals->max)
    internals->max = internals->current;

  if (internals->config.on_call) {
    allocationType t;
    t.insize = oldsize;
    t.outsize = newsize;
    t.trace = da;
    t.iptr = ptr;
    t.optr = res;
    internals->config.on_call(&t);
  }
  return res;
}
#endif // MY_DEBUG_ALLOCATOR_C
