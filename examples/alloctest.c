#include "../debugallocator.h"
#include "../macros.h"
#include "../mylist.h"
#include "../print.h"

int main(void) {
  var_ local = debugAllocator(
      allocator = stdAlloc,
      log = stdout,
  );
  defer { debugAllocatorDeInit(local); };
  mList(int) list = mList_init(local, int);
  mList_pushArr(
      list,
      ((int[]){5, 8, 7, 9, 5, 8, 7, 9})
  );
  for_each_((var_ element, mList_vla(list)), {
    println("{}", (int)element);
  });
  println("list footprint : {}", sizeof(*mList_vla(list)));

  return 0;
}
#include "../wheels.h"
