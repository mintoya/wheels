#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.h"
#include "../printers/slices.c"
#include "../printers/vason.c"
// #include "../types.h"
#include "../vason.h"
#include <stddef.h>
#include <uchar.h>

void testAllocation2(AllocatorV allocator) {
  My_allocator *arena = arena_new_ext(allocator, 1024);
  int *i = aCreate(arena, int, 8);
  i[0] = 8;
  aFree(arena, i);
}
void testAllocation(AllocatorV allocator) {
  int *i = aCreate(allocator, int);
  i[0] = 8;
  testAllocation2(allocator);
}
int main(void) {
  Arena_scoped *s = arena_new_ext(defaultAlloc, 1024);
  My_allocator *local = debugAllocatorInit(s);
  testAllocation(local);
  print("{} leaks detected", debugAllocatorDeInit(local));
  return 0;
}
#include "../wheels.h"
