#if !defined(FBA_ALLOCATOR_H)
  #define FBA_ALLOCATOR_H (1)
  #include "../allocator.h"

void *_fba_fn(void *allocator, void *ptr, usize oldsize, usize newsize, char *file, usize line);
void *_fba_alloc_nullable(allocfn allocator, usize size);

typedef struct {
  void *(*fn)(void *, void *, usize, usize, char *, usize);
  usize capacity, offset, count;
  u8 *buffer;
} FBA_State;

static inline usize FBA_current(allocfn allocator) {
  FBA_State *f = (typeof(f))allocator;
  return f->offset;
}
static inline void FBA_reset(allocfn allocator) {
  FBA_State *f = (typeof(f))allocator;
  f->offset = 0;
  f->count = 0;
}
static inline void FBA_init(u8 *buffer, usize size, FBA_State res[1]) {
  assert(buffer == (typeof(buffer))lineup((uptr)buffer, alignof(myAlign)));
  res->fn = _fba_fn;
  res->capacity = size;
  res->offset = 0;
  res->count = 0;
  res->buffer = (u8 *)buffer;
}

  #define fba_buffer(buffer)                         \
    struct {                                         \
      FBA_State s[1];                                \
      typeof(buffer) buff;                           \
      _Static_assert(                                \
          sizeof(typeof(buffer)) >= sizeof(myAlign), \
          "buffer cant store a single object"        \
      );                                             \
    }
  #define fba_initBuffer(buffer)                                 \
    (FBA_init((u8 *)buffer.buff, sizeof(buffer.buff), buffer.s), \
     (allocfn)buffer.s)

static inline allocfn fba_new(allocfn allocator, usize size) {
  typedef struct {
    FBA_State s[1];
    alignas(myAlign) u8 x[];
  } fbuffer;
  var_ r = (fbuffer *)vcall(allocator, fn, (nullptr, 0, size + sizeof(fbuffer), __FILE__, __LINE__));
  FBA_init(r->x, size, r->s);
  return (allocfn)r->s;
}
static inline void fba_del(allocfn allocator, allocfn fba) {
  typedef struct {
    FBA_State s[1];
    alignas(myAlign) u8 x[];
  } fbuffer;
  vcall(allocator, fn, ((void *)fba, sizeof(fbuffer) + ((fbuffer *)fba)->s->capacity, 0, __FILE__, __LINE__));
}
bool _fba_has(allocfn allocator, void *ptr);

#endif // FBA_ALLOCATOR_H
#if (defined FBA_ALLOCATOR_C && FBA_ALLOCATOR_C == 1) || \
    (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #undef FBA_ALLOCATOR_C
  #define FBA_ALLOCATOR_C (2)

bool _fba_has(allocfn allocator, void *ptr) {
  FBA_State *f = (typeof(f))allocator;
  return !(((uintptr_t)ptr) & (alignof(myAlign) - 1)) &&
         (u8 *)ptr >= f->buffer &&
         (u8 *)ptr - f->buffer < f->offset;
}

void *_fba_alloc_nullable(allocfn allocator, usize size) {
  FBA_State *f = (typeof(f))allocator;
  assert(!((uptr)f->buffer % alignof(myAlign)));
  size = lineup(size, alignof(myAlign));
  if (f->offset + size > f->capacity)
    return nullptr;
  assert(!(f->offset % alignof(myAlign)));
  void *res = f->buffer + f->offset;
  f->offset += size;
  f->count++;
  assert(!((uptr)res % alignof(myAlign)));
  return res;
}

void *_fba_fn(void *allocator, void *ptr, usize oldsize, usize newsize, char *file, usize line) {
  if (!newsize) {
    if (!ptr) return nullptr;
    assert(_fba_has((allocfn)allocator, ptr));
    FBA_State *f = (typeof(f))allocator;
    f->count--;
    if (!f->count)
      f->offset = 0;
    return nullptr;
  }

  void *res = _fba_alloc_nullable((allocfn)allocator, newsize);
  assert(res && "buffer probably ran out of space");

  if (ptr && oldsize) {
    memcpy(res, ptr, MIN$(oldsize, newsize));
    _fba_fn(allocator, ptr, oldsize, 0, file, line);
  }

  return res;
}

#endif // FBA_ALLOCATOR_C
