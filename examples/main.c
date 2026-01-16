#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.h"
#include "../printers/slices.c"
#include "../printers/vason.c"
#include "../types.h"
#include "../vason.h"
#include <stddef.h>

c8 pointstr[] = {
#embed "../vson/elements.vson"
};
// c8 pointstr[] = {
// #embed "../vson/point.json"
// };
int main(void) {
  Arena_scoped *s = arena_new_ext(pageAllocator, 1);
  My_allocator *local = debugAllocatorInit(s);

  slice(c8) pointData = slice_stat(pointstr);
  auto vson = parseStr(local, pointData);
  println(
      "input {} :\n{slice(c8)}\n point:\n{vason_container}",
      pointData.len, pointData, vson
  );
  println("{} objects", vson.objects.len);

  aFree(local, vson.objects.ptr);

  print("{} leaks detected", (ssize)debugAllocatorDeInit(local));
  return 0;
}
#include "../wheels.h"
