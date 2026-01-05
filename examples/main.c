#include "../arenaAllocator.h"
#include "../hhmap.h"
#include "../hlmap.h"
// #include "../hmap_arena.h"
#include "../print.h"
#include "../wheels.h"

int main(void) {
  Arena_scoped *local = arena_new_ext(pageAllocator, 1);

  HHMap_scoped *small_map = HHMap_new(sizeof(int), sizeof(int), local, 32);
  for (int i = 0; i < 64; i++) {
    int v = i * i;
    HHMap_set(small_map, &i, &v);
  }
  bool all = true;
  for (int i = 0; i < 64; i++) {
    int v;
    assertMessage(HHMap_getSet(small_map, &i, &v));
    println("{}->{}", i, v);
  }
  assertMessage(all, "hhmap couldnt retrieve all values ");
  HLMap_scoped *small_map2 = HLMap_new(sizeof(int), sizeof(int), local, 32);
  for (int i = 0; i < 64; i++) {
    int v = i * i;
    HLMap_set(small_map2, &i, &v);
  }
  all = true;
  for (int i = 0; i < 64; i++) {
    int v;
    assertMessage(HLMap_getSet(small_map2, &i, &v));
    println("{}->{}", i, v);
  }
  assertMessage(all, "hhmap couldnt retrieve all values ");

  println("arena size: {}", arena_footprint(local));
  println("map 1 footprint: {}", HHMap_footprint(small_map));
  println("map 2 footprint: {}", HLMap_footprint(small_map2));

  print("[");
  for (u32 i = 0; i < small_map2->ulist.length; i++)
    print("{wchar}", (*(u8 *)List_getRef(&small_map2->ulist, i)) ? L'▓' : L'░');
  println("]");
  HLMap_reload(&small_map2, 1.5);
  print("[");
  for (u32 i = 0; i < small_map2->ulist.length; i++)
    print("{wchar}", (*(u8 *)List_getRef(&small_map2->ulist, i)) ? L'▓' : L'░');
  println("]");
  return 0;
}
