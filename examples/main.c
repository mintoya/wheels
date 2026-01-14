#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.c"
#include "../printers/slices.c"
#include "../printers/vason.c"
#include "../types.h"
#include "../vason.h"
#include <stddef.h>

int main(void) {
  My_allocator* local = debugAllocatorInit(defaultAlloc);
  aCreate(local, int);
  debugAllocatorDeInit(local);
  assertMessage(false);
  return 0;
}
#include "../wheels.h"
