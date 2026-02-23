#include "debugallocator.h"
#include "vason_arr.h"
int main(void) {
  My_allocator *local = debugAllocatorInit(
          .allocator = stdAlloc,
          .log = stdout,
          .track_total = 1,
          .track_trace = 1,
  );

  defer { debugAllocatorDeInit(local); };
  c8 chars[] = "{hello:world}";
}
#include "wheels.h"
