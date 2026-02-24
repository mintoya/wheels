#include "debugallocator.h"
#include "my-list.h"
#include "shortList.h"
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
  slice(c8) str = mList_slice(chars);
  // println("input:\n{fptr}\n", str);
  vason_container c = vason_parseString(local, str);
  defer { vason_container_free(c); };

  for (auto i = 1; i < nargs; i++) {
    if (args[i][0] >= '0' && args[i][0] <= '9')
      c = vason_get(c, atoi(args[i]));
    else
      c = vason_get(c, fptr_CS(args[i]));
  }
  // vason_print(c, 0);
  println("{vason_container}", c);
  println(
      "indexes :{}\n"
      "tables  :{}\n"
      "tags    :{}\n"
      "text    :{}\n",
      sizeof(*msList_vla(c.indexes)),
      sizeof(*msList_vla(c.tables_strings)),
      sizeof(*msList_vla(c.tags)),
      c.text.len,
  );
}

// (')233
// indexes :1668
// tables  :3336
// tags    :417
// text    :4023
//
// (')233
// indexes :834
// tables  :1668
// tags    :417
// text    :4026
