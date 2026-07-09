#include "../allocators/arenaAllocator.h"
#include "../allocators/debugallocator.h"
#include "../print.h"
#include "../vason_arr.h"

#include "../wheels.h"
slice(c8) read_stdin(AllocatorV allocator) {
  usize size = 0;
  c8 *data = NULL;
  usize capacity = 4096;
  size = 0;
  data = (c8 *)aAlloc(allocator, capacity);
  usize bytes;
  while ((bytes = fread(data + size, 1, capacity - size, stdin)) > 0) {
    size += bytes;
    if (size == capacity) {
      data = (c8 *)aResize(allocator, data, capacity, (capacity * 2));
      capacity *= 2;
    }
  }
  return (slice(c8)){.ptr = data, .len = size};
}
bool isDigit(c8 c) { return c >= '0' && c <= '9'; }
#include "../cmdline_parser.h"
cmd_main(
    int nargs,
    char **args,
    (bool, lazy, ("--lazy", "-l"), "lazily parse", false),
    (bool, help, ("--help", "-h"), "print help message", false),
) {
  if (help) {
    cmd_usage(args[0]);
    return 0;
  } else {
    args++;
    nargs--;
  };

  slice(c8) input = read_stdin(stdAlloc);
  defer { aFree(stdAlloc, input.ptr, input.len); };

  AllocatorV local = stdAlloc;

  vason_container parsed =
      lazy
          ? vason_parseString_Lazy(local, input)
          : vason_parseString(local, input);
  println("{vason_container}", parsed);
  vason_container *f = &parsed;
  defer { vason_container_free(*f); };
  vason_index current = parsed.current;
  if (nargs)
    foreach (char *cptr, vla(*VLAP(args, nargs))) {
      println("getting {} from {vason_container}", cptr, P$(parsed, ({$.current = current;$; })));
      current =
          isDigit(cptr[0])
              ? vason_get_idx(&parsed, current, atoi(cptr))
              : vason_get_str(&parsed, current, fptr_CS(cptr));
    }
  if (lazy)
    vason_lazy_expand(&parsed, current);
  parsed.current = current;
  println("{vason_container}", parsed);
  return 0;
}
