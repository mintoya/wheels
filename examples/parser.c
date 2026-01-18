#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.h"
#include "../printers/slices.c"
#include "../printers/vason.c"
#include "../types.h"
#include "../vason.h"
#include <stddef.h>
#include <stdio.h>

slice(c8) read_stdin() {
  mList(c8) reading = mList_init(defaultAlloc, c8, 500);
  i32 c = 0;
  while ((c = fgetc(stdin)) != EOF)
    mList_push(reading, (c8)c);
  slice(c8) result = {mList_len(reading), mList_arr(reading)};
  List_forceResize((List *)reading, mList_len(reading));
  aFree(((List *)reading)->allocator, reading);
  return result;
}
int main(void) {
  Arena_scoped *local = arena_new_ext(pageAllocator, 1024);
  slice(c8) input = read_stdin();
  println("input len({}): {fptr}", input.len, input);
  vason_container parsed = parseStr(local, input);
  print( "parsed:\n{vason_container}", parsed);
}

#include "../wheels.h"
