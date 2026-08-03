#if !defined FBA_FALLBACK_H
  #define FBA_FALLBACK_H (1)
  #include "../allocator.h"
//{main helper for arenas
  #include "arenaAllocator.h"
static allocfn initarena(void *arg) {
  struct {
    allocfn alloc;
    usize size;
  } *m = (typeof(m))arg;
  return arena_new_ext(m ? m->alloc : stdAlloc, m ? m->size : 1024);
}
static void deinitarena(allocfn allocator, void *) { arena_cleanup(allocator); }
//}

void *_fbafb_fn(allocfn allocator, void *ptr, usize oldsize, usize newsize, const char *f, uint l);
struct fbab {
  struct allocfns fn[1];
  u8 *mem;
  usize offset, cap, count;
  void *ctx;
  allocfn allocator;
  fnptrof((void *), allocfn) init;
  fnptrof((allocfn, void *), void) deinit;
};
allocfn fbafb_init(
    struct fbab mem[1],
    u8 *buffer,
    usize size,
    void *ctx,
    itypeof(struct fbab, init) initializer,
    itypeof(struct fbab, deinit) deinitializer
);
void fbafb_deinit(allocfn allocator);

  #if !defined __cplusplus
    #define fbafb_buffer(buffer)              \
      struct {                                \
        struct fbab allocator[1];             \
        alignas(myAlign) typeof(buffer) buff; \
      }
  #else
template <typename T>
struct fbafb_buffer_t {
  struct fbab allocator[1];
  alignas(myAlign) T buff;
};
    #define fbafb_buffer(buffer) fbafb_buffer_t<buffer>

  #endif
  #define fbafb_initBuffer(buffer, ctx, init, deinit) fbafb_init(buffer.allocator, (u8 *)buffer.buff, sizeof(buffer.buff), ctx, init, deinit);

test_fn(fbafb_remain) {
  let b = (fbafb_buffer(myAlign[1])){};
  let alloc = fbafb_initBuffer(b, nullptr, nullptr, nullptr);
  // no deinit
  let i = acreate(alloc, int);
  *i = 1;
  adestroy(alloc, i);
}
test_fn(fbafb_grow) {
  let b = (fbafb_buffer(myAlign[1])){};
  let alloc = fbafb_initBuffer(b, nullptr, initarena, deinitarena);
  defer { fbafb_deinit(alloc); };
  let i = acreate(alloc, int[1000]);
  (*i)[0] = 1;
  adestroy(alloc, i);
}

#endif
#if (defined FBA_FALLBACK_C && FBA_FALLBACK_C == 1) || \
    (defined __INCLUDE_LEVEL__ && __INCLUDE_LEVEL__ == 0)
  #undef FBA_FALLBACK_C
  #define FBA_FALLBACK_C (2)
  #include "../assertMessage.h"
allocfn fbafb_init(
    struct fbab mem[1],
    u8 *buffer,
    usize size,
    void *ctx,
    itypeof(struct fbab, init) initializer,
    itypeof(struct fbab, deinit) deinitializer
) {
  assertMessage(!((uptr)buffer & (alignof(myAlign) - 1)));
  *mem = (typeof(*mem)){
      {_fbafb_fn},
      buffer,
      0,
      size,
      0,
      ctx,
      nullptr,
      initializer,
      deinitializer
  };
  return (allocfn)mem;
}
void fbafb_deinit(allocfn allocator) {
  let it = (struct fbab *)allocator;
  if (it->allocator) it->deinit(it->allocator, it->ctx);
  *it = (typeof(*it)){};
}
void *_fbafb_fn(allocfn allocator, void *ptr, usize oldsize, usize newsize, const char *f, uint l) {
  oldsize = lineup(oldsize, alignof(myAlign));
  newsize = lineup(newsize, alignof(myAlign));
  let it = (struct fbab *)allocator;

  if (!newsize) {
    if (!ptr) return nullptr;
    assertMessage(!((uptr)ptr & (alignof(myAlign) - 1)));
    let u = (uptr)ptr;
    if (u >= (uptr)it->mem && u < it->offset + (uptr)it->mem) {
      it->count--;
      if (!it->count) it->offset = 0;
      else if (it->mem + it->offset == (u8 *)ptr + oldsize) it->offset -= oldsize;
    } else {
      assertMessage(it->allocator);
      vcall(it->allocator, fn, (ptr, oldsize, 0, f, l));
    }
    return nullptr;
  }

  if (it->offset + newsize <= it->cap) {
    let res = it->mem + it->offset;
    it->count++;
    it->offset += newsize;
    if (ptr && oldsize) {
      memcpy(res, ptr, MIN$(oldsize, newsize));
      _fbafb_fn(allocator, ptr, oldsize, 0, f, l);
    }
    return res;
  };

  it->allocator = it->allocator ?: it->init(it->ctx);

  if (ptr) {
    let u = (uptr)ptr;
    if (u >= (uptr)it->mem && u < it->offset + (uptr)it->mem) {
      void *res = vcall(it->allocator, fn, (nullptr, 0, newsize, f, l));
      memcpy(res, ptr, MIN$(oldsize, newsize));
      _fbafb_fn(allocator, ptr, oldsize, 0, f, l);
      return res;
    }
  }

  return vcall(it->allocator, fn, (ptr, oldsize, newsize, f, l));
}
#endif
