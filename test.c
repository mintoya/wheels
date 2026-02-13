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
  println(
      "ff? {x}\n"
      "fa? {x}\n"
      "af? {x}",
      0xff, 0xfa, 0xaf
  );
}
#include "wheels.h"
