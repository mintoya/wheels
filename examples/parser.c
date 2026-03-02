#include "../debugallocator.h"
#include "../print.h"
#include "../vason_arr.h"
#include <ctype.h>

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
  mList(char *) argslist = mList_init(stdAlloc, char *, nargs);
  bool lazy = false;
  for (auto i = 1; i < nargs; i++) {
    if (args[i][0] == '-') {
      if (fptr_eq(fptr_CS(args[i] + 1), fp("-lazy")))
        lazy = true;
    } else
      mList_push(argslist, args[i]);
  }

  My_allocator *local = debugAllocatorInit(
      track_total = true,
      log = stderr,
  );
  defer { debugAllocatorDeInit(local); };

  slice(c8) input = read_stdin(stdAlloc);
  defer { aFree(stdAlloc, input.ptr); };

  vason_container parsed =
      lazy
          ? vason_parseString_Lazy(local, input)
          : vason_parseString(local, input);
  defer { vason_container_free(parsed); };
  mList_foreach(
      argslist,
      char *, cptr,
      if (isdigit(*cptr))
          parsed = vason_get(parsed, atoi(cptr));
      else parsed = vason_get(parsed, fptr_CS(cptr));
  );
  println("{vason_container}", parsed);
}
#include "../wheels.h"
