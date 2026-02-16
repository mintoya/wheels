#include "arenaAllocator.h"
#include "allocator.h"
#include "debugallocator.h"
#include "my-list.h"
#include "mytypes.h"
#include "print.h"

int main(void) {
  Arena_scoped *test = arena_new_ext(stdAlloc, 512);
  My_allocator *dbg = debugAllocatorInit(
          .allocator = test,
          .log = stdout, .track_total = 1
  );
  mList(int) ilist = mList_init(dbg, int);

  for (int i = 0; i < 100; i++)
    mList_push(ilist, i * i);

  for (int i = -1; i < 101; i++)
    print("{},", mList_getOr(ilist, i, -999));
  println();
  mList_deInit(ilist);
  debugAllocatorDeInit(dbg);
}
#include "wheels.h"
