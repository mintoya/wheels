#include "assertMessage.h" // debug symbols
#include "hhmap.h"

My_allocator *debugAllocatorInit(AllocatorV allocator);
void debugAllocatorDeInit(My_allocator *allocator);

//
//
//
//
//
struct tracedata {
  void *trace[5];
  usize alloc_size;
};
typedef struct {
  // hmap(void *, struct tracedata) map;
  AllocatorV actualAllocator;
  // usize max, current;
} debugAllocatorInternals;

void *debugAllocator_alloc(AllocatorV allocator, usize size);
void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size);
void debugAllocator_free(AllocatorV allocator, void *ptr);

usize debugAllocator_size(AllocatorV allocator, void *ptr);

debugAllocatorInternals *initializeInternals(AllocatorV allocator) {
  debugAllocatorInternals *internals = aCreate(allocator, debugAllocatorInternals);
  *internals = (debugAllocatorInternals){
      .actualAllocator = allocator,
      // .map = hmap_init(allocator, void *, struct tracedata),
      // .current = 0,
      // .max = 0,
  };
  return internals;
}
My_allocator *debugAllocatorInit(AllocatorV allocator) {
  My_allocator *res = aAlloc(allocator, sizeof(My_allocator));
  *res = (My_allocator){
      .alloc = debugAllocator_alloc,
      .ralloc = debugAllocator_realloc,
      .free = debugAllocator_free,
      .size = allocator->size,
      .arb = initializeInternals(allocator),
  };
  return res;
}
void debugAllocatorDeInit(My_allocator *allocator) {
  auto internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  // HHMap_free((void *)internals->map);
  aFree(realAllocator, internals);
  aFree(realAllocator, allocator);
}

void *debugAllocator_alloc(AllocatorV allocator, usize size) {
  auto internals = (debugAllocatorInternals *)allocator->arb;

  AllocatorV realAllocator = internals->actualAllocator;
  void *res = aAlloc(realAllocator, size);
  return res;
}
void *debugAllocator_realloc(AllocatorV allocator, void *ptr, usize size) {
  auto internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  void *res = aRealloc(realAllocator, ptr, size);
  return res;
}
void debugAllocator_free(AllocatorV allocator, void *ptr) {
  auto internals = (debugAllocatorInternals *)allocator->arb;
  AllocatorV realAllocator = internals->actualAllocator;
  return aFree(realAllocator, ptr);
}
