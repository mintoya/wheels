#include "../arenaAllocator.h"
#include "../print.h"
#include <stddefer.h>
//
#include "../debugallocator.h"
#include "../macros.h"
#include "../mytypes.h"
#include "../printers/slices.c"
#include "../vason_arr.h"
#include <stddef.h>

c8 pointstr[] = {
#embed "../vson/keys.kon"
};
int main(void) {
  Arena_scoped *s = arena_new_ext(stdAlloc, 1);

  My_allocator *local = debugAllocator(
      log = stdout,
      allocator = stdAlloc,
  );
  defer print("{} leaks detected", (isize)debugAllocatorDeInit(local));

  slice(c8) pointData = slice_stat(pointstr);
  auto vson = vason_parseString(local, pointData);
  defer aFree(local, vson.objects.ptr);

  println(
      "input {} :\n{slice(c8)}\n point:\n{vason_container}",
      pointData.len, pointData, vson
  );
  println("{} objects", msList_len(vson.tables_strings));

  return 0;
}
#include "../wheels.h"
