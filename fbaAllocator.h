#if !defined(FBA_ALLOCATOR_H)
  #define FBA_ALLOCATOR_H (1)
  #include "allocator.h"
  #include "mytypes.h"
  #include "stdalign.h"
  #include <assert.h>
  #include <stddef.h>
void *_fba_alloc(AllocatorV allocator, usize size, char *, usize);
void _fba_free(AllocatorV allocator, void *ptr, usize size, char *, usize);
void *_fba_alloc_nullable(AllocatorV allocator, usize size);
void _fba_print(AllocatorV allocator);
const My_allocator FBA_prototype[1] = {
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
  res->count = 0;
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
  #include <stdio.h>
void _fba_free(AllocatorV allocator, void *ptr, usize size, char *, usize) {
  size = aAlloc_align(size);
  FBA_State *f = (typeof(f))allocator;
  assert("ptr is outside buffer" && (u8 *)ptr >= (u8 *)f->buffer);
  assert("ptr is outside buffer" && (u8 *)ptr < (u8 *)f->buffer + f->offset);
  if (((u8 *)ptr + size) == f->offset + f->buffer)
    f->offset -= size;
  f->count--;
  if (!f->count)
    f->offset = 0;
}
void *_fba_alloc_nullable(AllocatorV allocator, usize size) {
  FBA_State *f = (typeof(f))allocator;
  size = aAlloc_align(size);
  if (f->offset + size > f->capacity)
    return nullptr;
  void *res = f->buffer + f->offset;
  f->offset += size;
  f->count++;
  return res;
}

void *_fba_alloc(AllocatorV allocator, usize size, char *, usize) {
  void *res = _fba_alloc_nullable(allocator, size);
  assert(res);
  return res;
}

#endif // FBA_ALLOCATOR_C
