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

slice(c8) read_stdin(AllocatorV allocator) {
  mList(c8) reading = mList_init(allocator, c8, 500);
  i32 c = 0;
  while ((c = fgetc(stdin)) != EOF)
    mList_push(reading, (c8)c);
  slice(c8) result = {mList_len(reading), mList_arr(reading)};
  List_forceResize((List *)reading, mList_len(reading));
  aFree(((List *)reading)->allocator, reading);
  return result;
}
int main(int nargs, char *args[nargs]) {
  Arena_scoped *local = arena_new_ext(pageAllocator, 4096);
  // My_allocator *local = debugAllocatorInit(arena);
  slice(c8) input = read_stdin(local);
  vason_container parsed = parseStr(local, input);
  vason_object result = parsed.top;
  for (usize i = 1; i < nargs; i++) {
    // println("getting {cstr}", args[i]);
    if (result.tag == vason_ARR) {
      result = vason_get(parsed, result, (usize)atoi(args[i]));
    } else {
      result = vason_get(parsed, result, (slice(c8)){.ptr = (u8 *)args[i], .len = strlen(args[i])});
    }
  }
  vason_contianer c = parsed;
  c.top = result;
  println("{vason_container}", c);
  aFree(local, parsed.objects.ptr);
  aFree(local, input.ptr);
  // debugAllocatorDeInit(local);
}

#include "../wheels.h"
