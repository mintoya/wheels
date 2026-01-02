#include "../arenaAllocator.h"
#include "../hhmap.h"
#include "../hmap_arena.h"
#include "../print.h"
#include "../wheels.h"

int main(void) {
  Arena_scoped *local = arena_new_ext(pageAllocator, 1);

  HHMap_scoped *small_map = HHMap_new(sizeof(int), sizeof(int), local, 4);
  for (int i = 0; i < 10; i++) {
    int v = i * i;
    HHMap_set(small_map, &i, &v);
  }
  assertMessage(HHMap_count(small_map) == 10);
  bool all = true;
  for (int i = 0; i < 10; i++) {
    int v;
    HHMap_getSet(small_map, &i, &v);
    valFullElse(
        HHMap_getSet(small_map, &i, &v),
        { all = false;v = -999; },
        false
    );
    println("{}->{}", i, v);
  }
  assertMessage(all, "hhmap couldnt retrieve all values ");

  return 0;
}
