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
  println("input : {fptr}", srt);
  vason_container c = vason_parseString(local, srt);
  defer { vason_container_free(c); };
  println("output:");
  vason_print(c, 0);
  c = vason_get(c, fp("a"));
  println("getting a : ");
  vason_print(c, 0);
}
