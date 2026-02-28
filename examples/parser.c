#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.h"
#include "../mytypes.h"
#include "../printers/slices.c"
#include "../vason_arr.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>

slice(c8) read_stdin(AllocatorV allocator) {
  mList(c8) reading = mList_init(allocator, c8, 500);
  defer { aFree(allocator, reading); };
  {
    i32 c = 0;
    while ((c = fgetc(stdin)) != EOF)
      mList_push(reading, (c8)c);
  }
  mList_setCap(reading, mList_len(reading) ?: 1);
  return (slice(c8))mList_slice(reading);
}
int main(int nargs, char *args[nargs]) {
  Arena_scoped *local = arena_new_ext(stdAlloc, 4096);

  slice(c8) input = read_stdin(local);
  vason_container parsed = vason_parseString(local, input);
  for (usize i = 1; i < nargs; i++) {
    if (isdigit(args[i][0]))
      parsed = vason_get(parsed, atoi(args[i]));
    else
      parsed = vason_get(parsed, fptr_CS(args[i]));
  }
  println("{vason_container}", parsed);
}

#include "../wheels.h"
