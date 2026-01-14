#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.c"
#include "../printers/slices.c"
#include "../printers/vason.c"
#include "../types.h"
#include "../vason.h"
#include <stddef.h>

int main(void) {
  // Arena_scoped *local = arena_new_ext(defaultAlloc, 1024);
  auto local = defaultAlloc;
  const c8 efile[] = {
#embed "../vason/elements.vason"
  };
  slice(c8) string = {sizeof(efile), (c8 *)efile};
  println("input :\n{slice(c8)}", string);
  auto c = parseStr(local, string);
  println("{vason_container}", c);
  aFree(local, c.objects.ptr);
  mHmap_scoped(int, size_t) ist = mHmap_init(local, int, size_t);
  mHmap_set(ist, 6, (size_t)7);
  size_t *t = mHmap_get(ist, 6);
  println("6 : {}", t ? *t : -999);

  return 0;
}
#include "../wheels.h"
