#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.h"
#include "../mytypes.h"
#include "../printers/slices.c"
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
  defer { arena_cleanup(local); };

  slice(c8) input = read_stdin(local);
  vason_container parsed = parseStr(local, input);
  for (usize i = 1; i < nargs; i++) {
    if (parsed.top.tag == vason_ARR) {
      parsed = vason_get(parsed, atoi(args[i]));
    } else {
      parsed = vason_get(parsed, ((slice(c8)){.ptr = (u8 *)args[i], .len = strlen(args[i])}));
    }
  }
  println("{vason_container}", parsed);
  aFree(local, parsed.objects.ptr);
  aFree(local, input.ptr);
}

#include "../wheels.h"
