#include "../allocator.h"
#include "../hxmap.h"

typedef struct trackingallocator trackingallocator;
struct trackingallocator {
  My_allocator self[0];
  AllocatorV backing;
  mxmap(void *, usize) track;
};
