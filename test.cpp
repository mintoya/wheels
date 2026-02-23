#include "debugallocator.h"
#include "vason_arr.h"
#include "wheels.h"
int main(void) {
  My_allocator *local = debugAllocatorInit(
          .allocator = stdAlloc,
          .track_total = 1,
          .log = stdout,
  );

  defer { debugAllocatorDeInit(local); };
  c8 chars[] = {
#embed "vson/experimental.vason"
  };
  slice(c8) srt = slice_stat(chars);
  println("input:\n{fptr}\n",srt);
  vason_container c = vason_parseString(local, srt);
  defer { vason_container_free(c); };
  vason_print(&c, c.current, 0);
}
