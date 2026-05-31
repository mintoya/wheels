#if !defined(TSA_ALLOCATOR_H)
  #define TSA_ALLOCATOR_H (1)
  #include "../allocator.h"
  #include <threads.h>

typedef struct TSA_State {
  My_allocator allocator[1];
  AllocatorV underlying;
  mtx_t mutex[1];
} TSA_State;

void *_tsa_alloc(AllocatorV allocator, size_t size, char *file, usize line);
void _tsa_free(AllocatorV allocator, void *ptr, usize size, char *file, usize line);
void *_tsa_resize(AllocatorV allocator, void *ptr, size_t old_size, size_t new_size, char *file, usize line);
size_t _tsa_size(AllocatorV allocator, void *ptr);

static const My_allocator TSA_prototype[1] = {(My_allocator){
    .alloc = _tsa_alloc,
    .free = _tsa_free,
    .resize = _tsa_resize,
    .size = _tsa_size,
}};
static inline AllocatorV TSA_init(AllocatorV underlying) {
  TSA_State *res = aCreate(underlying, TSA_State);
  memcpy(res->allocator, TSA_prototype, sizeof(res->allocator));
  res->underlying = underlying;
  mtx_init(res->mutex, mtx_plain);
  return res->allocator;
}
static inline void TSA_deinit(AllocatorV allocator) {
  TSA_State *ts = (typeof(ts))allocator;
  mtx_destroy(ts->mutex);
  var_ a = ts->underlying;
  aFree(a, ts, sizeof(*ts));
}

#endif // TSA_ALLOCATOR_H

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define TSA_ALLOCATOR_C (1)
#endif

#ifdef TSA_ALLOCATOR_C

void *_tsa_alloc(AllocatorV allocator, size_t size, char *file, usize line) {
  TSA_State *tsa = (typeof(tsa))allocator;
  mtx_lock(tsa->mutex);
  defer { mtx_unlock(tsa->mutex); };
  return (aAlloc)(tsa->underlying, size, file, line);
}

void _tsa_free(AllocatorV allocator, void *ptr, usize size, char *file, usize line) {
  TSA_State *tsa = (typeof(tsa))allocator;
  mtx_lock(tsa->mutex);
  defer { mtx_unlock(tsa->mutex); };
  return (aFree)(tsa->underlying, ptr, size, file, line);
}

void *_tsa_resize(AllocatorV allocator, void *ptr, size_t old_size, size_t new_size, char *file, usize line) {
  TSA_State *tsa = (typeof(tsa))allocator;

  mtx_lock(tsa->mutex);
  defer { mtx_unlock(tsa->mutex); };
  return (aResize)(tsa->underlying, ptr, old_size, new_size, file, line);
}

size_t _tsa_size(AllocatorV allocator, void *ptr) {
  TSA_State *tsa = (typeof(tsa))allocator;
  size_t res = 0;

  if (tsa->underlying->size) {
    mtx_lock(tsa->mutex);
    res = tsa->underlying->size(tsa->underlying, ptr);
    mtx_unlock(tsa->mutex);
  }
  return res;
}

#endif // TSA_ALLOCATOR_C
