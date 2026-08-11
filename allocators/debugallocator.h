#if !defined MY_DEBUG_ALLOCATOR_H
  #define MY_DEBUG_ALLOCATOR_H
  #include "../allocator.h"
  #include "../assertMessage.h"
  #include "../macros.h"
  #include "../print/print_pre.h"
  #include "cballocator.h"
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
typedef struct debugStats {
  usize max_memory, current_memory, total_calls, total_active_allocations;
} dbga_stats;
typePrinter(dbga_stats) {
  PUTS("{max:");
  USENAMEDPRINTER("usize", in.max_memory);
  PUTS(",");
  PUTS("current:");
  USENAMEDPRINTER("usize", in.current_memory);
  PUTS(",");
  PUTS("active:");
  USENAMEDPRINTER("usize", in.total_active_allocations);
  PUTS(",");
  PUTS("calls:");
  USENAMEDPRINTER("usize", in.total_calls);
  PUTS("}");
}
dbga_stats debugAllocator_stats(allocfn allocator);
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
typedef struct debugallocattorIterator_state {
  usize idx;
  const allocfn _self;
} debugallocattorIterator_state;
debugallocattorIterator_state debugallocator_iterator_init(allocfn allocator);
typedef struct {
  void *ptr;
  struct tracedata trace;
} debugAllocator_iter_item;
debugAllocator_iter_item debugallocator_iterator_cast(const debugallocattorIterator_state *);
void debugallocator_iterator_increase(debugallocattorIterator_state *);
bool debugallocator_iterator_valid(const debugallocattorIterator_state *);

NAMESPACE_STRUCT(
    debugallocator_iterator,
    (init, &debugallocator_iterator_init),
    (cast, &debugallocator_iterator_cast),
    (increase, &debugallocator_iterator_increase),
    (valid, &debugallocator_iterator_valid),
);

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
  let _self = (debugAllocator_state *)(h->cbself->udata);
  let in = h->insize;
  let out = h->outsize;
  typedef enum : u8 {
    ALLOC = 0b01,
    RESIZE = 0b11,
    FREE = 0b10,
  } amode;
  let n = (amode)((!!in) << 1) | ((!!out) << 0);
  usize tracked_in = 0;
  switch (n) {
    case RESIZE: {
      assertMessage(p);
      let ptr = dbgallocator_map_get(_self->map, h->inptr);
      assertMessage(ptr, "allocator lost pointer");
      tracked_in = ptr->size;
      dbgallocator_map_rem(_self->map, h->inptr);
      dbgallocator_map_set(_self->map, p, (struct tracedata){h->filename, h->linenumber, out});
    } break;
    case ALLOC: {
      assertMessage(p);
      assertMessage(!dbgallocator_map_get(_self->map, p), "allocator gave used pointer");
      dbgallocator_map_set(_self->map, p, (struct tracedata){h->filename, h->linenumber, out});
    } break;
    case FREE: {
      assertMessage(!p);
      let ptr = dbgallocator_map_get(_self->map, h->inptr);
      assertMessage(ptr, "allocator lost pointer");
      tracked_in = ptr->size;
      dbgallocator_map_rem(_self->map, h->inptr);
    } break;
    default:
      unreachable();
  }
  _self->stats.total_calls++;
  _self->stats.total_active_allocations += (n == ALLOC) - (n == FREE);
  _self->stats.current_memory += (isize)h->outsize - (isize)tracked_in;
  _self->stats.max_memory = MAX$(_self->stats.max_memory, _self->stats.current_memory);
  if (_self->onalloc) _self->onalloc(h->inptr, in, out, p, h->filename, h->linenumber);
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
  let _self = (debugAllocator_state *)(cba->udata);
  defer { adestroy(cba->allocator, _self); };
  int leaks = 0;

  foreach (let kv, vtable(dbgallocator_map_iterator, _self->map)) {
    leaks++;
    adestroy(cba->allocator, (u8(*)[kv.val->size])kv.key);
  }
  dbgallocator_map_freem(_self->map[0]);
  return leaks;
}

struct debugStats debugAllocator_stats(allocfn afn) {
  let cba = (callbackallocatorbuffer *)afn;
  let _self = (debugAllocator_state *)(cba->udata);
  return _self->stats;
}
int debugAllocator_clear(allocfn afn) {
  let cba = (callbackallocatorbuffer *)afn;
  let _self = (debugAllocator_state *)(cba->udata);
  int leaks = 0;
  foreach (let kv, vtable(dbgallocator_map_iterator, _self->map)) {
    leaks++;
    adestroy(cba->allocator, (u8(*)[kv.val->size])kv.key);
  }
  dbgallocator_map_clear(_self->map);
  return leaks;
}
debugallocattorIterator_state debugallocator_iterator_init(allocfn allocator) {
  let cba = (callbackallocatorbuffer *)allocator;
  let _self = (debugAllocator_state *)(cba->udata);
  let it = dbgallocator_map_iter_init(_self->map);
  return (debugallocattorIterator_state){it.current, allocator};
}
debugAllocator_iter_item debugallocator_iterator_cast(const debugallocattorIterator_state *s) {
  let cba = (callbackallocatorbuffer *)s->_self;
  let _self = (debugAllocator_state *)(cba->udata);
  let state = (dbgallocator_map_iter_state){
      .map = _self->map,
      .current = s->idx,
  };
  let it = dbgallocator_map_iter_cast(&state);
  return (debugAllocator_iter_item){.ptr = it.key, .trace = *it.val};
}
void debugallocator_iterator_increase(debugallocattorIterator_state *s) {
  let cba = (callbackallocatorbuffer *)s->_self;
  let _self = (debugAllocator_state *)(cba->udata);
  let state = (dbgallocator_map_iter_state){
      .map = _self->map,
      .current = s->idx,
  };
  dbgallocator_map_iter_increase(&state);
  s->idx = state.current;
}
bool debugallocator_iterator_valid(const debugallocattorIterator_state *s) {
  let cba = (callbackallocatorbuffer *)s->_self;
  let _self = (debugAllocator_state *)(cba->udata);
  let state = (dbgallocator_map_iter_state){
      .map = _self->map,
      .current = s->idx,
  };
  return dbgallocator_map_iter_valid(&state);
}
#endif // MY_DEBUG_ALLOCATOR_C
