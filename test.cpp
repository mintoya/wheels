#include "debugallocator.h"
#include "vason_arr.h"
#include "wheels.h"
int main(void) {
  My_allocator *local = debugAllocatorInit(
          .allocator = stdAlloc,
          .log = stdout,
          .track_total = 1,
          .track_trace = 1,
  );

  defer { debugAllocatorDeInit(local); };
  c8 chars[] = {
#embed "vson/experimental.vason"
  };
  slice(c8) srt = slice_stat(chars);
  vason_container c = vason_parseString(local, srt);
  defer { vason_container_free(c); };
  // c = vason_get(c, ((vason_tag[]){vason_tags.pair, vason_tags.string}), fp("hello"));
  vason_print(&c, c.current, 0);
}
