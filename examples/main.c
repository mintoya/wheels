#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.h"
#include "../printers/slices.c"
#include "../printers/vason.c"
// #include "../types.h"
#include "../vason.h"
#include <stddef.h>

void testAllocation2(AllocatorV allocator) {
  int *i = aCreate(allocator, int, 8);
  i[0] = 8;
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
  int leaks = debugAllocatorDeInit(local);
  assertMessage(
      leaks == 0,
      "%i leaks detected", leaks
  );
  return 0;
}
#include "../wheels.h"
