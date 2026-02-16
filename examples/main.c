#include "../arenaAllocator.h"
#include "../print.h"
#include <stddefer.h>
//
#include "../debugallocator.h"
#include "../mltypes.h"
#include "../printers/slices.c"
#include "../vason.h"
#include <stddef.h>

// c8 pointstr[] = {
// #embed "../vson/keys.kon"
// };
// c8 pointstr[] = {
// #embed "../vson/lua.lua"
// };
// c8 pointstr[] = {
// #embed "../vson/elements.vson"
// };
c8 pointstr[] = {
#embed "../vson/capabilities.vason"
};
int main(void) {
  Arena_scoped *s = arena_new_ext(pageAllocator, 1);

  My_allocator *local = debugAllocatorInit(
          .log = stdout,
          .allocator = stdAlloc,
  );
  defer print("{} leaks detected", (isize)debugAllocatorDeInit(local));

  slice(c8) pointData = slice_stat(pointstr);
  auto vson = parseStr(local, pointData);
  defer aFree(local, vson.objects.ptr);

  println(
      "input {} :\n{slice(c8)}\n point:\n{vason_container}",
      pointData.len, pointData, vson
  );
  println("{} objects", vson.objects.len);

  return 0;
}
#include "../wheels.h"
