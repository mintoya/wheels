#include "my-list.h"
#include "omap.h"
#include "print.h"
#include "stringList.h"

#define for_range(a, b, i) for (typeof(a) i = a; a < b ? i < b : i > b; a < b ? i++ : i--)
#include "debugallocator.h"
int main(void) {
  My_allocator *local = debugAllocatorInit(stdAlloc);
  stringList *sl = stringList_new(local, 1);
  stringList_append(sl, fp_from("hello"));
  println("{}", stringList_get(sl, 0));
  println("len {}",stringList_len(sl));
  stringList_free(sl);
  OMap *map = OMap_new(local, 10);
  OMap_set(map, fp_from("hello"), fp_from("world"));
  OMap_set(map, fp_from("world"), fp_from("hello"));
  OMap_set(map, fp_from("a"), fp_from("b"));
  OMap_set(map, fp_from("world"), fp_from("hello"));
  OMap_set(map, fp_from("world"), nullFptr);
  for (int i = 0; i < stringList_len(map->data); i++)
    println("{} -> {} ", i, stringList_get(map->data, i));

  println("footprint: {}", OMap_footprint(map));
  println("{} -> {}", fp_from("a"), OMap_get(map, fp_from("a")));
  println("{} -> {}", fp_from("hello"), OMap_get(map, fp_from("hello")));
  OMap_free(map);
  debugAllocatorDeInit(local);
}
#include "wheels.h"
