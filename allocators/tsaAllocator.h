#if !defined(TSA_ALLOCATOR_H)
  #define TSA_ALLOCATOR_H (1)
  #include "../allocator.h"
  #include "../thread_help.h"
  #include <string.h>

typedef struct TSA_State {
  struct allocfns allocator[1];
  allocfn underlying;
  mtx_t mutex[1];
} TSA_State;

void *_tsa_allocator(allocfn, void *, usize, usize, const char *, const uint);
static inline allocfn TSA_init(allocfn underlying) {
  TSA_State *res = acreate(underlying, TSA_State);
  let af = _tsa_allocator;
  memcpy(res->allocator, &af, sizeof(res->allocator));
  res->underlying = underlying;
  mtx_init(res->mutex, mtx_plain);
  return res->allocator;
}
static inline void TSA_deinit(allocfn allocator) {
  TSA_State *ts = (typeof(ts))allocator;
  mtx_destroy(ts->mutex);
  var_ a = ts->underlying;
  adestroy(a, ts);
}

#endif // TSA_ALLOCATOR_H

#if (defined TSA_ALLOCATOR_C && TSA_ALLOCATOR_C == 1) || (defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0)
  #undef TSA_ALLOCATOR_C
  #define TSA_ALLOCATOR_C (2)
void *_tsa_allocator(allocfn fn, void *op, usize in, usize out, const char *fl, const uint ln) {
  TSA_State *tsp = (typeof(tsp))fn;
  mtx_lock(tsp->mutex);
  defer { mtx_unlock(tsp->mutex); };
  return vcall(tsp->underlying, fn, (op, in, out, fl, ln));
}
#endif // TSA_ALLOCATOR_C
