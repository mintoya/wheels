#include "../allocator.h"
#include "../arenaAllocator.h"
#include "../hhmap.h"
#include "../my-list.h"
#include "../print.h"
int main(void) {
  Arena_scoped *localArena = arena_new_ext(pageAllocator, 1024);
  mList(int) list = mList_init(localArena, int);
  // null since the list isnt that long
  // otherwise points *inside* list
  int *first = mList_get(list, 0);
  mList_push(list, 4);
  mList_push(list, 5);
  mList_push(list, 6);
  mList_push(list, 7);
  mList_foreach(list, int, v, {
    println("{}", v);
  });
  mHmap(int, int) map = mHmap_init(localArena, int, int);
  mHmap_set(map, 1, 1);
  mHmap_set(map, 2, 4);
  mHmap_set(map, 3, 9);
  mHmap_set(map, 4, 16);
  // null since key doesnt exist
  // also in the map otherwise
  int *i = mHmap_get(map, 6);
  mHmap_foreach(map, int, key, int, val, {
    println("{} -> {}", key, val);
  });
}
#include "../wheels.h"
