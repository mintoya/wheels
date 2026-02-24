#include "debugallocator.h"
#include "vason_arr.h"
#include "wheels.h"
#include <stdio.h>
int main(void) {
  My_allocator *local = debugAllocatorInit(
          .allocator = stdAlloc,
          .track_total = 1,
          .log = stdout,
  );
  defer { debugAllocatorDeInit(local); };

  mList(c8) chars = mList_init(local, c8);
  defer { mList_deInit(chars); };

  {
    int c;
    while ((c = fgetc(stdin)) != EOF)
      mList_push(chars, c);
  }

  slice(c8) srt = slice_stat(*mList_vla(chars));
  println("input:\n{fptr}\n", srt);
  vason_container c = vason_parseString(local, srt);
  defer { vason_container_free(c); };
  vason_print(&c, c.current, 0);
}
