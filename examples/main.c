#include "../arenaAllocator.h"
#include "../debugallocator.h"
#include "../mylist.h"
#include "../print.h"
#include "../wheels.h"
#include <stddefer.h>
int main(void) {
  AllocatorV *s = arena_new_ext(stdAlloc, 1024);
  defer { arena_cleanup(s); };

  AllocatorV local = debugAllocator(
      log = stderr,
      allocator = stdAlloc,
  );
  defer { print("{} leaks detected\n", (isize)debugAllocatorDeInit(local)); };

  mList(int) ints = mList_init(local, int, 1000);
  defer { mList_deInit(ints); };

  for (each_RANGE(int, i, 0, 100))
    mList_push(ints, i);

  var_ ints2 = mList_map(
      ints, local, i, ({
        struct intbuf {
          char buf[12];
        };
        struct intbuf bufa;
        snprintf(bufa.buf, sizeof(bufa), "%d", i * i);
        bufa;
      })
  );

  defer { mList_deInit(ints2); };
  foreach (var_ i, mList_vla(ints2))
    println("{cstr}", (c8 *)i.buf);
  return 0;
}
