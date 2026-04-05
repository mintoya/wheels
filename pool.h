#include "macros.h"
#if !defined(MY_OBJECT_POOLS_H)
  #define MY_OBJECT_POOLS_H (1)
  #include "sList.h"
  #include <stddef.h>
typedef struct objPool objPool;
#endif

#if defined(__INCLUDE_LEVEL__) && __INCLUDE_LEVEL__ == 0
  #define MY_OBJECT_POOLS_C (1)
#endif

#if defined(MY_OBJECT_POOLS_C)
typedef struct {
  usize len, cap;
  u8 *buffer;
} objectPool_item;
typedef struct objPool {
  AllocatorV allocator;
  usize initSize;
  msList(objectPool_item) pools;
} objPool;
objPool *objPool_new(AllocatorV allocator, usize initSize) {
  objPool *res = aCreate(allocator, objPool);
  res->allocator = allocator;
  res->pools = msList_init(res->allocator, objectPool_item);
  res->initSize = initSize;
  return res;
}
void *objPool_push(objPool *p, usize itemSize) {
  for_each_P((var_ pool, msList_vla(p->pools)), {
    if (pool->len < pool->cap)
      return pool->len++ * itemSize + pool->buffer;
  });
  objectPool_item i =
      (typeof(i)){
          .len = 0,
          .cap = p->initSize,
          .buffer = aCreate(p->allocator, u8, p->initSize * itemSize),
      };
  msList_push(p->allocator, p->pools, i);
  var_ ip = msList_get(p->pools, msList_len(p->pools) - 1);

  return ip->len++ * itemSize + ip->buffer;
}

#endif
