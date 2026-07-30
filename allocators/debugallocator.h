#if !defined MY_DEBUG_ALLOCATOR_H
  #define MY_DEBUG_ALLOCATOR_H
  #include "../allocator.h"
  #include "../macros.h"
  #include "../print/print_pre.h"

struct tracedata {
  const char *fn;
  usize ln;
  usize size;
};
struct dbgAlloc_config {
  allocfn allocator;
  fnptrof((void *, usize, usize, void *, const char *, uint), void) onalloc;
};
/**
 * `@param` **allocator**  allocator
 *      - backend allocator, it will also store itself here
 * `@return` debug allocator
 */
allocfn debugAllocatorInit(struct dbgAlloc_config);
struct debugStats {
  usize max_memory, current_memory, total_calls, total_active_allocations;
};
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
struct debugStats debugAllocator_stats(allocfn allocator);
int debugAllocator_clear(allocfn allocator);
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
int debugAllocatorDeInit(allocfn);

  #include "../tests.h"
test_fn(debugallocator_test) {
  let alloc = debugAllocator(.allocator = allocator);
  acreate(alloc, int[5]);
  aresize(alloc, acreate(alloc, int[5]), int[2]);
  let p = acreate(alloc, int[5]);
  let statsa = debugAllocator_stats(alloc);
  aresize(alloc, p, int[2]);
  let statsb = debugAllocator_stats(alloc);
  test_assert(statsa.total_active_allocations == statsb.total_active_allocations);
  test_assert(statsa.max_memory == statsb.max_memory);
  test_assert(statsa.current_memory > statsb.current_memory);
  test_assert(statsa.total_calls < statsb.total_calls);
  test_inteq(debugAllocatorDeInit(alloc), 3);
}

#endif // MY_DEBUG_ALLOCATOR_H
#if (defined MY_DEBUG_ALLOCATOR_C && MY_DEBUG_ALLOCATOR_C == 1) || \
    defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #undef MY_DEBUG_ALLOCATOR_C
  #define MY_DEBUG_ALLOCATOR_C (2)

  #define mapconfig dbgallocator_map, void *, struct tracedata, ((iptr)k), ((iptr)a - (iptr)b)
  #include "../incmap.h"
typedef struct {
  struct debugStats stats;
  fnptrof((void *, usize, usize, void *, const char *, uint), void) onalloc;
  dbgallocator_map map[1];
} debugAllocator_state;
  #include "cballocator.h"
void _dbga_cba(const callbackallocatorhandle *h) {
  let in = h->insize;
  let out = h->outsize;
  typedef enum : u8 {
    ALLOC = 0b01,
    RESIZE = 0b11,
    FREE = 0b10,
  } amode;
  let n = ((!!in) << 1) | ((!!out) << 0);
  switch (n) {
    case ALLOC:
      assertMessage(!h->inptr);
      break;
    case FREE:
    case RESIZE:
      assertMessage(h->inptr);
      break;
    default:
      assertMessage(false, "invalid call from %s line %u: (%p , %zu , %zu)", h->filename, h->linenumber, 0xDEADC0DE, in, out);
  }
}
void _dbga_cbb(const callbackallocatorhandle *h, void *p) {
  let slef = (debugAllocator_state *)(h->cbself->udata);
  let in = h->insize;
  let out = h->outsize;
  typedef enum : u8 {
    ALLOC = 0b01,
    RESIZE = 0b11,
    FREE = 0b10,
  } amode;
  let n = (amode)((!!in) << 1) | ((!!out) << 0);
  switch (n) {
    case RESIZE: {
      assertMessage(p);
      let ptr = dbgallocator_map_get(slef->map, h->inptr);
      assertMessage(ptr, "allocator lost pointer");
      dbgallocator_map_rem(slef->map, h->inptr);
      dbgallocator_map_set(slef->map, p, (struct tracedata){h->filename, h->linenumber, out});
    } break;
    case ALLOC: {
      assertMessage(p);
      assertMessage(!dbgallocator_map_get(slef->map, p), "allocator gave used pointer");
      dbgallocator_map_set(slef->map, p, (struct tracedata){h->filename, h->linenumber, out});
    } break;
    case FREE: {
      assertMessage(!p);
      let ptr = dbgallocator_map_get(slef->map, h->inptr);
      assertMessage(ptr, "allocator lost pointer");
      dbgallocator_map_rem(slef->map, h->inptr);
    } break;
    default:
      unreachable();
  }
  slef->stats.total_calls++;
  slef->stats.total_active_allocations += (n == ALLOC) - (n == FREE);
  slef->stats.current_memory += (isize)h->outsize - (isize)h->insize;
  slef->stats.max_memory = MAX$(slef->stats.max_memory, slef->stats.current_memory);
  if (slef->onalloc) slef->onalloc(h->inptr, in, out, p, h->filename, h->linenumber);
}
allocfn debugAllocatorInit(struct dbgAlloc_config config) {
  let allocator = config.allocator;
  let res = cba_init(
      allocator,
      _dbga_cba,
      _dbga_cbb,
      acreate(allocator, debugAllocator_state)
  );
  let stat = (debugAllocator_state *)(((callbackallocatorbuffer *)res)->udata);
  stat->stats = (typeof(stat->stats)){};
  stat->onalloc = config.onalloc;
  dbgallocator_map_newm(allocator, 3, stat->map);
  return res;
}
int debugAllocatorDeInit(allocfn afn) {
  defer { cba_deinit(afn); };
  let cba = (callbackallocatorbuffer *)afn;
  let slef = (debugAllocator_state *)(cba->udata);
  int leaks = 0;

  foreach (let kv, vtable(dbgallocator_map_iterator, slef->map)) {
    leaks++;
    adestroy(cba->allocator, (u8(*)[kv.val->size])kv.key);
  }
  dbgallocator_map_freem(slef->map[0]);
  return leaks;
}

struct debugStats debugAllocator_stats(allocfn afn) {
  let cba = (callbackallocatorbuffer *)afn;
  let slef = (debugAllocator_state *)(cba->udata);
  return slef->stats;
}
int debugAllocator_clear(allocfn afn) {
  let cba = (callbackallocatorbuffer *)afn;
  let slef = (debugAllocator_state *)(cba->udata);
  int leaks = 0;
  foreach (let kv, vtable(dbgallocator_map_iterator, slef->map)) {
    leaks++;
    adestroy(cba->allocator, (u8(*)[kv.val->size])kv.key);
  }
  dbgallocator_map_clear(slef->map);
  return leaks;
}
#endif // MY_DEBUG_ALLOCATOR_C
