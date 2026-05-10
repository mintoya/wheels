#if !defined(TSA_ALLOCATOR_H)
  #define TSA_ALLOCATOR_H (1)
  #include "allocator.h"
  #include "mytypes.h"
  #include <string.h>
  #include <threads.h> // Using standard C11 threads for mtx_t

typedef struct {
  My_allocator allocator[1];
  AllocatorV underlying;
  mtx_t mutex;
} TSA_State;

void *_tsa_alloc(AllocatorV allocator, size_t size, char *file, usize line);
void _tsa_free(AllocatorV allocator, void *ptr, usize size, char *file, usize line);
size_t _tsa_size(AllocatorV allocator, void *ptr);

const My_allocator TSA_prototype[1] = {(My_allocator){
    .alloc = _tsa_alloc,
    .free = _tsa_free,
    .size = _tsa_size,
}};

extern inline void TSA_init(AllocatorV underlying, TSA_State res[1]) {
  memcpy(res->allocator, TSA_prototype, sizeof(res->allocator));
  res->underlying = underlying;
  mtx_init(&res->mutex, mtx_plain);
}

extern inline void TSA_deinit(TSA_State res[1]) { mtx_destroy(&res->mutex); }

#endif // TSA_ALLOCATOR_H

// ---------------------------------------------------------

#if (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #define TSA_ALLOCATOR_C (1)
#endif

#ifdef TSA_ALLOCATOR_C

void *_tsa_alloc(AllocatorV allocator, size_t size, char *file, usize line) {
  TSA_State *tsa = (typeof(tsa))allocator;
  mtx_lock(&tsa->mutex);
  void *res = tsa->underlying->alloc(tsa->underlying, size, file, line);
  mtx_unlock(&tsa->mutex);
  return res;
}

void _tsa_free(AllocatorV allocator, void *ptr, usize size, char *file, usize line) {
  TSA_State *tsa = (typeof(tsa))allocator;
  mtx_lock(&tsa->mutex);
  tsa->underlying->free(tsa->underlying, ptr, size, file, line);
  mtx_unlock(&tsa->mutex);
}

size_t _tsa_size(AllocatorV allocator, void *ptr) {
  TSA_State *tsa = (typeof(tsa))allocator;
  size_t res = 0;

  if (tsa->underlying->size) {
    mtx_lock(&tsa->mutex);
    res = tsa->underlying->size(tsa->underlying, ptr);
    mtx_unlock(&tsa->mutex);
  }
  return res;
}

#endif // TSA_ALLOCATOR_C
