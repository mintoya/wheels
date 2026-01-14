#define PRINTER_LIST_TYPENAMES
#include "../arenaAllocator.h"
#include "../debugallocator.c"
#include "../fptr.h"
#include "../hhmap.h"
#include "../print.h"
#include "../wheels.h"
#include <stddef.h>
#include <stdint.h>

int main(void) {
  My_allocator *local = debugAllocatorInit(defaultAlloc);
  do {
    mList(int) list = mList_init(local, typeof(list(NULL)));
    mList_push(list, 5);
    mList_push(list, 8);
    mList_push(list, 7);
    mList_push(list, 9);
    mList_push(list, 5);
    mList_push(list, 8);
    mList_push(list, 7);
    mList_push(list, 9);
    mList_foreach(list, int, element, {
      println("{}", (int)element);
    });
    println("list footprint : {}", List_headArea((List *)list));
    println("{cstr}", (char *)"hello world");

  } while (0);
  debugAllocatorDeInit(local);
  return 0;
}
