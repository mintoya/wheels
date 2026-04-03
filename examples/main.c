#include "../arenaAllocator.h"
#include "../debugallocator.h"
#include "../mylist.h"
#include "../print.h"
#include "../wheels.h"
#include <stdcountof.h>
#include <stddefer.h>
#include <stdio.h>
int main(void) {
  AllocatorV s = arena_new_ext(stdAlloc, 1024);
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
  var_ ints2 = mList_init(local, struct { char buf[12]; });

  for_each_((var_ i, mList_vla(ints)), {
    mList_iType(ints2) l = {};
    snprintf(l.buf, countof(l.buf), "%d", i * i);
    mList_push(ints2, l);
  });
  defer { mList_deInit(ints2); };

  for_each_(
      (var_ i, mList_vla(ints2)), {
        println("{cstr}", (c8 *)i.buf);
      }
  );
  return 0;
}
