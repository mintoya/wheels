#if !defined FBA_FALLBACK_H
  #define FBA_FALLBACK_H (1)
  #include "../allocator.h"
  #include "../assertMessage.h"
//{main helper for arenas
  #include "arenaAllocator.h"
static AllocatorV initarena(void *arg) {
  let m = (struct {AllocatorV alloc ; usize size; } *)arg;
  return arena_new_ext(m ? m->alloc : stdAlloc, m ? m->size : 1024);
}
static void deinitarena(AllocatorV allocator, void *) { arena_cleanup(allocator); }
//}

void *_fbafb_alloc(AllocatorV, usize, char *, usize);
void _fbafb_free(AllocatorV, void *, usize, char *, usize);
struct fbab {
  My_allocator vt[1];
  u8 *mem;
  usize offset, cap, count;
  void *ctx;
  AllocatorV allocator;
  fnptrof((void *), AllocatorV) init;
  fnptrof((AllocatorV, void *), void) deinit;
};
AllocatorV fbafb_init(
    struct fbab mem[1],
    u8 *buffer,
    usize size,
    void *ctx,
    itypeof(struct fbab, init) initializer,
    itypeof(struct fbab, deinit) deinitializer
);
void fbafb_deinit(AllocatorV allocator);

  #define fbafb_buffer(buffer)  \
    struct {                    \
      struct fbab allocator[1]; \
      typeof(buffer) buff;      \
    }
  #define fbafb_initBuffer(buffer, ctx, init, deinit) fbafb_init(buffer.allocator, (u8 *)buffer.buff, sizeof(buffer.buff), ctx, init, deinit);

test_fn(fbafb_remain) {
  let b = (fbafb_buffer(myAlign[1])){};
  let alloc = fbafb_initBuffer(b, nullptr, nullptr, nullptr);
  // no deinit
  let i = aCreate(alloc, int);
  *i = 1;
  aFree(alloc, i, sizeof(int));
}
test_fn(fbafb_grow) {
  let b = (fbafb_buffer(myAlign[1])){};
  let alloc = fbafb_initBuffer(b, nullptr, initarena, deinitarena);
  defer { fbafb_deinit(alloc); };
  let i = aCreate(alloc, int, 1000);
  *i = 1;
  aFree(alloc, i, sizeof(int) * 1000);
}

#endif
#if (defined FBA_FALLBACK_C && FBA_FALLBACK_C == 1) || (defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0)
  #define FBA_FALLBACK_C (2)
AllocatorV fbafb_init(
    struct fbab mem[1],
    u8 *buffer,
    usize size,
    void *ctx,
    itypeof(struct fbab, init) initializer,
    itypeof(struct fbab, deinit) deinitializer
) {
  assertMessage(!((uptr)buffer & (alignof(myAlign) - 1)));
  *mem = (typeof(*mem)){
      {_fbafb_alloc, _fbafb_free},
      buffer,
      0,
      size,
      0,
      ctx,
      nullptr,
      initializer,
      deinitializer
  };
  return mem->vt;
}
void fbafb_deinit(AllocatorV allocator) {
  let it = (struct fbab *)allocator;
  if (it->allocator) it->deinit(it->allocator, it->ctx);
  *it = (typeof(*it)){};
}
void *_fbafb_alloc(AllocatorV allocator, usize size, char *, usize) {
  let it = (struct fbab *)allocator;
  size = lineup(size, alignof(myAlign));
  if (it->offset + size <= it->cap) {
    let res = it->mem + it->offset;
    it->offset += size;
    return res;
  };
  return aAlloc(
      (it->allocator = it->allocator ?: it->init(it->ctx)),
      size
  );
}
void _fbafb_free(AllocatorV allocator, void *ptr, usize oldsize, char *f, usize l) {
  assertMessage(!((uptr)ptr & (alignof(myAlign) - 1)));
  let it = (struct fbab *)allocator;
  let u = (uptr)ptr;
  if (u >= (uptr)it->mem && u < it->offset + (uptr)it->mem) {
    it->count--;
    if (!it->count) it->offset = 0;
    else if (
        it->mem + it->offset ==
        (u8 *)ptr + lineup(oldsize, alignof(myAlign))
    ) it->offset -= lineup(oldsize, alignof(myAlign));
  } else (aFree)(
      (it->allocator) ?: (assertMessage(false), nullptr),
      ptr,
      oldsize,
      f,
      l
  );
}
#endif
