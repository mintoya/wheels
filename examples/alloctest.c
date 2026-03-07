#include "../debugallocator.h"
#include "../macros.h"
#include "../mylist.h"
#include "../print.h"

int main(void) {
  My_allocator *local = debugAllocator(
      allocator = stdAlloc,
      log = stdout,
  );
  defer { debugAllocatorDeInit(local); };
  mList(int) list = mList_init(local, int);
  __auto_type i = 1;
  mList_pushVla(
      list,
      ((int[]){5, 8, 7, 9, 5, 8, 7, 9})
  );
  mList_foreach(list, int, element, {
    println("{}", (int)element);
  });
  println("list footprint : {}", List_headArea((List *)list));

  return 0;
}
#include "../wheels.h"
