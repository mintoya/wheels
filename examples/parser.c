#include "../arenaAllocator.h"
#include "../debugallocator.h"
#include "../print.h"
#include "../vason_arr.h"
#include "../wheels.h"
#include <ctype.h>

slice(c8) read_stdin(AllocatorV allocator) {
  usize size = 0;
  c8 *data = NULL;
  usize capacity = 4096;
  size = 0;
  data = (c8 *)aAlloc(allocator, capacity);
  usize bytes;
  while ((bytes = fread(data + size, 1, capacity - size, stdin)) > 0) {
    size += bytes;
    if (size == capacity)
      data = (c8 *)aResize(allocator, data, (capacity += capacity));
  }
  return (slice(c8)){.ptr = data, .len = size};
}
int main(int nargs, char *args[nargs]) {
  mList(char *) argslist = mList_init(stdAlloc, char *, nargs);
  defer { mList_deInit(argslist); };
  bool lazy = false;
  for (auto i = 1; i < nargs; i++) {
    if (args[i][0] == '-') {
      if (fptr_eq(fptr_CS(args[i] + 1), fp("-lazy")))
        lazy = true;
    } else
      mList_push(argslist, args[i]);
  }

  slice(c8) input = read_stdin(stdAlloc);
  defer { aFree(stdAlloc, input.ptr); };

  AllocatorV local = stdAlloc;
  // My_allocator *local = debugAllocator(allocator = stdAlloc, track_total = 1);
  // defer { debugAllocatorDeInit(local); };

  vason_container parsed =
      lazy
          ? vason_parseString_Lazy(local, input)
          : vason_parseString(local, input);
  vason_container *f = &parsed; // clang defer
  defer { vason_container_free(*f); };
  mList_foreach(
      argslist,
      char *, cptr,
      parsed =
          isdigit(*cptr)
              ? *vason_get(&parsed, atoi(cptr))
              : *vason_get(&parsed, fptr_CS(cptr));
  );
  parsed =
      lazy
          ? *vason_lazy_expand(&parsed)
          : parsed;
  println("{vason_container}", parsed);
}
