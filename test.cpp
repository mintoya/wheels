#include "debugallocator.h"
#include "vason_arr.h"
#include "wheels.h"
#include <stdlib.h>
int main(int nargs, char *args[]) {
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
  slice(c8) str = slice_stat(*mList_vla(chars));
  println("input:\n{fptr}\n", str);
  vason c = vason_parseString(local, str);
  defer { c.free(); };

  for (auto i = 1; i < nargs; i++) {
    if (args[i][0] >= '0' && args[i][0] <= '9')
      c = c[atoi(args[i])];
    else
      c = c[args[i]];
  }
  vason_print(c, 0);
}
