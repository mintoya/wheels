#include "../allocator.h"
#include "../arenaAllocator.h"
#include "../hhmap.h"
#include "../my-list.h"
#include "../print.h"
int main(void) {
  Arena_scoped *localArena = arena_new_ext(pageAllocator, 1024);
  mList(int) list = mList_init(localArena, int);
  mList_push(list, 5);
  mList_push(list, 5);
  mList_push(list, 67);
  mList_foreach(list, int, v, {
    println("{}", v);
  });
  mHmap(int, int) map = mHmap_init(localArena, int, int);
  mHmap_set(map, 6, 7);
  mHmap_set(map, 8, 1);
  mHmap_set(map, 6, 7);
  mHmap_set(map, 7, 8);
  mHmap_foreach(map, int, key, int, val, {
    println("{} -> {}", key, val);
  });
  int *i = mHmap_get(map, 6);
}
#include "../wheels.h"
