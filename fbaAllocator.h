#if !defined(FBA_ALLOCATOR_H)
  #define FBA_ALLOCATOR_H (1)
  #include "allocator.h"
  #include "mytypes.h"
  #include "stdalign.h"
  #include <assert.h>
  #include <stddef.h>
void *_fba_alloc(AllocatorV allocator, usize size);
void _fba_free(AllocatorV allocator, void *ptr);
void *_fba_resize(AllocatorV allocator, void *oldptr, usize size);
usize _fba_size(AllocatorV allocator, void *ptr);
My_allocator FBA_prototype[1] = {
    (My_allocator){
        .alloc = _fba_alloc,
        .free = _fba_free,
        .resize = _fba_resize,
        .size = _fba_size,
    }
};

typedef struct {
  alignas(max_align_t) usize size;
  alignas(max_align_t) u8 mem[];
} FBA_Header;

typedef struct {
  My_allocator allocator[1];
  usize capacity, offset;
  alignas(max_align_t) u8 *buffer;
} FBA_State;
extern inline usize FBA_current(AllocatorV allocator) {
  FBA_State *f = (typeof(f))allocator;
  return f->offset;
}
extern inline void FBA_reset(AllocatorV allocator) {
  FBA_State *f = (typeof(f))allocator;
  f->offset = 0;
  // f->count = 0;
}
extern inline void FBA_init(max_align_t *buffer, usize size, FBA_State res[1]) {
  __builtin_memcpy(res->allocator, FBA_prototype, sizeof(res->allocator));
  res->capacity = size;
  res->offset = 0;
  // res->count = 0;
  res->buffer = (u8 *)buffer;
}
  #define FBA_static(size) ({                                                             \
    static max_align_t _buffer[(((size) + sizeof(FBA_State))) / sizeof(max_align_t) + 1]; \
    static FBA_State _res = {};                                                           \
    if (!_res.capacity)                                                                   \
      FBA_init(_buffer, sizeof(_buffer), &_res);                                          \
    _res.allocator;                                                                       \
  })

#endif // FBA_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define FBA_ALLOCATOR_C (1)
#endif

#ifdef FBA_ALLOCATOR_C
  #define FBA_FREE_BIT ((usize)1 << (sizeof(usize) * 8 - 1))
  #define FBA_SIZE_MASK (~FBA_FREE_BIT)

static inline int _fba_is_free(FBA_Header *h) { return (h->size & FBA_FREE_BIT) != 0; }

static inline usize _fba_get_size(FBA_Header *h) { return h->size & FBA_SIZE_MASK; }

static inline void _fba_set_free(FBA_Header *h) { h->size |= FBA_FREE_BIT; }

static inline void _fba_set_size(FBA_Header *h, usize size) {
  h->size = size; // clears free bit
}
void _fba_coallese(AllocatorV allocator) {
  FBA_State *f = (typeof(f))allocator;

  u8 *ptr = f->buffer;
  u8 *end = f->buffer + f->offset;

  FBA_Header *prev = NULL;

  while (ptr < end) {
    FBA_Header *curr = (FBA_Header *)ptr;
    usize curr_size = _fba_get_size(curr);

    u8 *next_ptr = ptr + sizeof(FBA_Header) + curr_size;

    if (prev && _fba_is_free(prev) && _fba_is_free(curr)) {
      usize new_size =
          _fba_get_size(prev) +
          sizeof(FBA_Header) +
          curr_size;

      _fba_set_size(prev, new_size);
      _fba_set_free(prev);

      ptr = (u8 *)prev;
      continue;
    }

    prev = curr;
    ptr = next_ptr;
  }

  while (f->offset > 0) {
    u8 *scan = f->buffer;
    FBA_Header *last = NULL;

    while (scan < f->buffer + f->offset) {
      FBA_Header *h = (FBA_Header *)scan;
      last = h;
      scan += sizeof(FBA_Header) + _fba_get_size(h);
    }

    if (!last || !_fba_is_free(last))
      break;

    f->offset -= sizeof(FBA_Header) + _fba_get_size(last);
  }
}
void *_fba_alloc(AllocatorV allocator, usize size) {
  assert(!(size & ((usize)1 << (sizeof(usize) * 8 - 1))));
  FBA_State *f = (typeof(f))allocator;
  usize innersize = sizeof(FBA_Header) + lineup(size, sizeof(max_align_t));
  FBA_Header *res = (typeof(res))(f->buffer + f->offset);
  res->size = lineup(size, sizeof(max_align_t));
  // f->count++;
  f->offset += innersize;
  assert("fba ran out of memory" && f->offset < f->capacity);
  return res->mem;
}
void _fba_free(AllocatorV allocator, void *ptr) {
  FBA_State *f = (typeof(f))allocator;
  assert("ptr is outside buffer" && ptr > (u8 *)f->buffer);
  assert("ptr is outside buffer" && ptr < (u8 *)f->buffer + f->offset);
  FBA_Header *h = (typeof(h))((u8 *)ptr - offsetof(FBA_Header, mem));
  if (h->mem + h->size == f->offset + f->buffer)
    f->offset -= sizeof(*h) + h->size;
  else {
    h->size |= ((usize)1 << (sizeof(usize) * 8 - 1));
    _fba_coallese(allocator);
  }
  // f->count--;
  // if (f->count == 0)
  //   f->offset = 0;
}
void *_fba_resize(AllocatorV allocator, void *oldptr, usize size) {
  FBA_State *f = (typeof(f))allocator;
  assert("ptr is outside buffer" && oldptr > (u8 *)f->buffer);
  assert("ptr is outside buffer" && oldptr < (u8 *)f->buffer + f->offset);
  FBA_Header *h = (typeof(h))((u8 *)oldptr - offsetof(FBA_Header, mem));
  if (h->mem + h->size == f->offset + f->buffer) {
    usize aligned_size = lineup(size, sizeof(max_align_t));
    f->offset += (aligned_size - h->size);
    h->size = aligned_size;
    assert(f->offset < f->capacity);
    return oldptr;
  }
  void *res = _fba_alloc(allocator, size);
  __builtin_memcpy(res, oldptr, h->size);
  _fba_free(allocator, oldptr);
  return res;
}
usize _fba_size(AllocatorV allocator, void *ptr) {
  FBA_State *f = (typeof(f))allocator;
  assert("ptr is outside buffer" && ptr > (u8 *)f->buffer);
  assert("ptr is outside buffer" && ptr < (u8 *)f->buffer + f->offset);
  FBA_Header *h = (typeof(h))((u8 *)ptr - offsetof(FBA_Header, mem));
  return h->size;
}
#endif // FBA_ALLOCATOR_C
