#include "../allocator.h"
#include "../allocators/debugallocator.h"
#include "../macros.h"
#include "../mylist.h"
#include "../print.h"

int main(void) {
  var_ local =
      debugAllocator(
              .allocator = stdAlloc,
              .log = stdout,
      );
  defer {
    // invoke custom printer for allocator stats
    println("{dbga-stats}", debugAllocator_stats(local));
    debugAllocatorDeInit(local);
  };
  mList(int) list = mList_init(local, int);
  mList_pushArr(
      list,
      ((int[]){5, 8, 7, 9, 5, 8, 7, 9})
  );
  foreach (var_ element, vlap(mList_vla(list))) {
    println("{}", (int)element);
  };
  println("list footprint : {}", sizeof(*mList_vla(list)));

  return 0;
}
#include "wheels/wheels.h"
