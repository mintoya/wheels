#include "my-list.h"
#include "omap.h"
#include "print.h"
#include "stringList.h"

#include "debugallocator.h"
#include <stdcountof.h>
#include <stdio.h>
static struct {
  typeof(OMap_set) *set;
  typeof(OMap_get) *get;
  typeof(OMap_new) *new;
  typeof(OMap_free) *free;
} omap = {
    OMap_set,
    OMap_get,
    OMap_new,
    OMap_free,
};
int main(void) {
  My_allocator *local = debugAllocatorInit(
      stdAlloc,
      (struct dbgAlloc_config){
          .log = stdout,
          // .track_total = 1,
          // .track_trace = 1,
      }
  );
  stringList *sl = stringList_new(local, 1);
  stringList_append(sl, fp_from("hello"));
  println("{}", stringList_get(sl, 0));
  println("len {}", stringList_len(sl));
  stringList_free(sl);
  OMap *map = omap.new(local, 10);
  OMap_set(map, fp("hello"), fp("world"));
  OMap_set(map, fp("world"), fp("hello"));
  OMap_set(map, fp("a"), fp("b"));
  OMap_set(map, fp("world"), fp("herro"));
  for (int i = 0; i < stringList_len(map->data); i++)
    println("{} -> {} ", i, stringList_get(map->data, i));

  println("footprint: {}", OMap_footprint(map));
  for (int i = 0; i < stringList_len(map->data) / 2; i++)
    println(
        "{} -> {}",
        stringList_get(map->data, i * 2),
        stringList_get(map->data, i * 2 + 1)
    );
  // OMap_free(map);
  int leaks = debugAllocatorDeInit(local);
  assertMessage(!leaks, "found %i leaks", leaks);
}
#include "wheels.h"
