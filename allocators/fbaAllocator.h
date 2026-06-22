#if !defined(FBA_ALLOCATOR_H)
  #define FBA_ALLOCATOR_H (1)
  #include "../allocator.h"
void *_fba_alloc(AllocatorV allocator, usize size, char *, usize);
void _fba_free(AllocatorV allocator, void *ptr, usize size, char *, usize);
void *_fba_alloc_nullable(AllocatorV allocator, usize size);
void _fba_print(AllocatorV allocator);
static const My_allocator FBA_prototype[1] = {
    (My_allocator){
        .alloc = _fba_alloc,
        .free = _fba_free,
        .resize = nullptr,
        .size = nullptr,
    }
};

typedef struct {
  My_allocator allocator[1];
  usize capacity, offset, count;
  u8 *buffer;
} FBA_State;
static inline usize FBA_current(AllocatorV allocator) {
  FBA_State *f = (typeof(f))allocator;
  return f->offset;
}
static inline void FBA_reset(AllocatorV allocator) {
  FBA_State *f = (typeof(f))allocator;
  f->offset = 0;
  f->count = 0;
}
static inline void FBA_init(u8 *buffer, usize size, FBA_State res[1]) {
  assert(buffer == (typeof(buffer))lineup((uptr)buffer, alignof(myAlign)));
  memcpy(res->allocator, FBA_prototype, sizeof(res->allocator));
  res->capacity = size;
  res->offset = 0;
  res->count = 0;
  res->buffer = (u8 *)buffer;
}

  #define fba_buffer(buffer)  \
    ((struct {                \
      FBA_State s[1];         \
      typeof(u8 buffer) buff; \
    }){})
  #define fba_initBuffer(buffer)                           \
    (FBA_init(buffer.buff, sizeof(buffer.buff), buffer.s), \
     buffer.s->allocator)
static inline AllocatorV fba_new(AllocatorV allocator, usize size) {
  typedef struct {
    FBA_State s[1];
    alignas(myAlign) u8 x[];
  } fbuffer;
  var_ r = (fbuffer *)aAlloc(allocator, size + sizeof(fbuffer));
  FBA_init(r->x, size, r->s);
  return r->s->allocator;
}
static inline void fba_del(AllocatorV allocator, AllocatorV fba) {
  typedef struct {
    FBA_State s[1];
    alignas(myAlign) u8 x[];
  } fbuffer;
  aFree(allocator, (void *)fba, sizeof(fbuffer) + ((fbuffer *)fba)->s->capacity);
}

#endif // FBA_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define FBA_ALLOCATOR_C (1)
#endif

#ifdef FBA_ALLOCATOR_C
void _fba_free(AllocatorV allocator, void *ptr, usize size, char *, usize) {
  FBA_State *f = (typeof(f))allocator;
  f->count--;
  if (!f->count)
    f->offset = 0;
}
void *_fba_alloc_nullable(AllocatorV allocator, usize size) {
  FBA_State *f = (typeof(f))allocator;
  assert(!((uptr)f->buffer % alignof(myAlign)));
  size = aAlloc_align(size);
  if (f->offset + size > f->capacity)
    return nullptr;
  assert(!(f->offset % alignof(myAlign)));
  void *res = f->buffer + f->offset;
  f->offset += size;
  f->count++;
  assert(!((uptr)res % alignof(myAlign)));
  return res;
}

void *_fba_alloc(AllocatorV allocator, usize size, char *, usize) {
  void *res = _fba_alloc_nullable(allocator, size);
  assert(res && "buffer probably ran out of space");
  return res;
}

#endif // FBA_ALLOCATOR_C
