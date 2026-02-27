#include "debugallocator.h"
#include "my-list.h"
#include "shortList.h"
#include "vason_arr.h"
#include "wheels.h"
#include <ctype.h>
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

  // slice(c8) str = slice_stat(u"]");

  vason_container c = vason_parseString(local, str);
  defer { vason_container_free(c); };

  for (auto i = 1; i < nargs; i++)
    c = isdigit(args[i][0])
            ? vason_get(c, atoi(args[i]))
            : vason_get(c, fptr_CS(args[i]));
  println("{vason_container}", c);
  println(
      "tables  :{}\n"
      "tags    :{}\n"
      "text    :{}\n",
      sizeof(*msList_vla(c.tables_strings)),
      sizeof(*msList_vla(c.tags)),
      c.text.len,
  );
}
