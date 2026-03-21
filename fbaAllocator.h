#ifndef FBA_ALLOCATOR_H
#define FBA_ALLOCATOR_H
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
  alignas(max_align_t) u8 mem[/*size*/];
} FBA_Header;

typedef struct {
  My_allocator allocator[1];
  usize capacity, offset, count;
  alignas(max_align_t) u8 buffer[];
} FBA_State;
extern inline usize FBA_current(AllocatorV allocator) {
  FBA_State *f = (typeof(f))allocator;
  return f->offset;
}
extern inline void FBA_reset(AllocatorV allocator) {
  FBA_State *f = (typeof(f))allocator;
  f->offset = 0;
  f->count = 0;
}
extern inline AllocatorV FBA_init(u8 *buffer, usize size) {
  FBA_State *res = (typeof(res))(buffer);
  __builtin_memcpy(res->allocator, FBA_prototype, sizeof(res->allocator));
  res->capacity = size - sizeof(*res);
  res->offset = 0;
  res->count = 0;
  return res->allocator;
}
#define FBA_static(size) ({                             \
  static u8 _buffer[((size) + sizeof(FBA_State))];      \
  AllocatorV _res = FBA_init(_buffer, sizeof(_buffer)); \
  _res;                                                 \
})

#endif // FBA_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
#define FBA_ALLOCATOR_C (1)
#endif

#ifdef FBA_ALLOCATOR_C
void *_fba_alloc(AllocatorV allocator, usize size) {
  FBA_State *f = (typeof(f))allocator;
  usize innersize = sizeof(FBA_Header) + lineup(size, sizeof(max_align_t));
  FBA_Header *res = (typeof(res))(f->buffer + f->offset); // FIXED: Removed + innersize
  res->size = lineup(size, sizeof(max_align_t));
  f->count++;
  f->offset += innersize;
  assert(f->offset < f->capacity);
  return res->mem;
}
void _fba_free(AllocatorV allocator, void *ptr) {
  FBA_State *f = (typeof(f))allocator;
  assert(ptr > (u8 *)f->buffer);
  assert(ptr < (u8 *)f->buffer + f->offset);
  FBA_Header *h = (typeof(h))((u8 *)ptr - offsetof(FBA_Header, mem));
  if (h->mem + h->size == f->offset + f->buffer)
    f->offset -= sizeof(*h) + h->size;
  f->count--;
  if (f->count == 0)
    f->offset = 0;
}
void *_fba_resize(AllocatorV allocator, void *oldptr, usize size) {
  FBA_State *f = (typeof(f))allocator;
  assert(oldptr > (u8 *)f->buffer);
  assert(oldptr < (u8 *)f->buffer + f->offset);
  FBA_Header *h = (typeof(h))((u8 *)oldptr - offsetof(FBA_Header, mem));
  if (h->mem + h->size == f->offset + f->buffer) {
    usize aligned_size = lineup(size, sizeof(max_align_t)); // FIXED: Align the new size
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
  assert(ptr > (u8 *)f->buffer);
  assert(ptr < (u8 *)f->buffer + f->offset);
  FBA_Header *h = (typeof(h))((u8 *)ptr - offsetof(FBA_Header, mem));
  return h->size;
}
#endif // FBA_ALLOCATOR_C
