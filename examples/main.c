#include "../arenaAllocator.h"
#include "../debugallocator.h"
#include "../hhmap.h"
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
  // println("{cstr}", slice.ptr);

  var_ imap = mHmap_init(local, int, int);
  defer { mHmap_deinit(imap); };

  foreach (int i, range(0, 10, 1))
    mHmap_set(imap, i, i * i);
  foreach (var_ v, iter(mHmap_iterator(imap, int)))
    println("{i32} -> {}", v->key, v->val);
  println("{mHmap:i32,i32}", imap);

  return 0;
}
