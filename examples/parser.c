#include "../debugallocator.h"
#include "../print.h"
#include "../vason_arr.h"
#include <ctype.h>

slice(c8) read_stdin(AllocatorV allocator) {
  struct stat st;
  usize size = 0;
  c8 *data = NULL;
  if (fstat(fileno(stdin), &st) == 0 && st.st_size > 0) {
    size = (usize)st.st_size;
    data = (c8 *)aAlloc(allocator, size);

    usize bytes_read = fread(data, 1, size, stdin);
    if (bytes_read < size) {
      data = (c8 *)aResize(allocator, data, bytes_read);
      size = bytes_read;
    }
  } else {
    usize capacity = 4096;
    size = 0;
    data = (c8 *)aAlloc(allocator, capacity);
    usize bytes;
    while ((bytes = fread(data + size, 1, capacity - size, stdin)) > 0) {
      size += bytes;
      if (size == capacity)
        data = (c8 *)aResize(allocator, data, (capacity += capacity));
    }
    data = (c8 *)aResize(allocator, data, size ?: 1);
  }

  return (slice(c8)){.ptr = data, .len = size};
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

  slice(c8) input = read_stdin(stdAlloc);
  // defer { aFree(stdAlloc, input.ptr); };

  vason_container parsed =
      lazy
          ? vason_parseString_Lazy(stdAlloc, input)
          : vason_parseString(stdAlloc, input);
  // defer { vason_container_free(parsed); };
  mList_foreach(
      argslist,
      char *, cptr,
      if (isdigit(*cptr))
          parsed = *vason_get(&parsed, atoi(cptr));
      else parsed = *vason_get(&parsed, fptr_CS(cptr));
  );
  println("{vason_container}", parsed);
}
#include "../wheels.h"
