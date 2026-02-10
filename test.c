#include "debugallocator.h"
#include "my-list.h"
#include "omap.h"
#include "print.h"
#include "printers/slices.c"
#include "stringList.h"
#include "types.h"
#include <malloc.h>
#include <stdcountof.h>
#include <stdio.h>

void(listWrite)(const c32 *c, mList(c32) wl, unsigned int len, bool flush) {
  List_appendFromArr((List *)wl, c, len);
  if (flush)
    mList_push(wl, 0);
}
#define fmt_print(allocator, slice, ...) ({                 \
  mList(c32) chars = mList_init(allocator, c32);            \
  print_wfO((outputFunction)listWrite, chars, __VA_ARGS__); \
  typeof(slice) res = {mList_len(chars), mList_arr(chars)}; \
  aFree(allocator, chars);                                  \
  res;                                                      \
})
int main(void) {
  My_allocator *local = debugAllocatorInit(
      stdAlloc,
      (struct dbgAlloc_config){
          .log = stdout,
          // .track_total = 1,
          .track_trace = 1,
      }
  );
  slice(c32) sn = fmt_print(local, sn, "hello {} world", 6767);
  println("{slice(c32)}", sn);
  int leaks = debugAllocatorDeInit(local);
}
#include "wheels.h"
