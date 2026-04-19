#include "../arenaAllocator.h"
#include "../debugallocator.h"
#include "../mylist.h"
#include "../print.h"
#include "../wheels.h"
int main(void) {
  AllocatorV s = arena_new_ext(stdAlloc, 1024);
  defer { arena_cleanup(s); };

  AllocatorV local = debugAllocator(
      log = stderr,
      allocator = stdAlloc,
  );
  defer { print("{} leaks detected\n", (isize)debugAllocatorDeInit(local)); };
  var_ slice = snprint(local, "hello {} {}", "world", 6);
  println("{cstr}", slice.ptr);

  return 0;
}
