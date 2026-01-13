#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.c"
#include "../vason.h"

typedef struct vason_container vason_container;
REGISTER_PRINTER(vason_container, {
  for (auto i = 0; i < LIST_LENGTH(in.objects); i++) {
    vason_object o = in.top;
    switch (o.tag) {
      case vason_ARR:
        PUTS(L"󰅨 ");
        PUTS(L"[");
        for (usize i = 0; i < o.data.list.len; i++) {
        }
        PUTS(L"]");
        break;
      case vason_MAP:
        PUTS(L"󱃖 ");
        PUTS(L"{");
        for (usize i = 0; i < o.data.object.len; i++) {
        }
        PUTS(L"}");
        break;
      case vason_STR:
        PUTS(L"󰅳 ");
        break;
    }
  }
});

#include "../types.h"

int main(void) {
  Arena_scoped *local = arena_new_ext(pageAllocator, 1);
  const u8 efile[] = {
#embed "../vason/elements.vason"
  };
  HMAP(i8, i32)
  map = HMAP_INIT(pageAllocator, i8, i32);
  HMAP_SET(map, (i8)6, 7);
  i32 *p = HMAP_GET(map, (i8)6);
  HMAP_FREE(map);

  slice(u8) string = slice_stat(efile);

  println("input :\n{fptr}", string);

  struct vason_container c = parseStr(local, string);

  println("{vason_container}", c);

  // debugAllocatorDeInit(local);
  // println("arena footprint: {}", arena_footprint(real));
  return 0;
}
#include "../wheels.h"
