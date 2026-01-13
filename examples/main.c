#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../debugallocator.c"
#include "../vason.h"

typedef struct vason_container vason_container;
REGISTER_PRINTER(vason_container, {
  for (auto i = 0; i < in.objects->length; i++) {
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

int main(void) {
  Arena_scoped *local = arena_new_ext(pageAllocator, 1);
  const u8 efile[] = {
#embed "../vason/elements.vason"
  };
  slice(u8) string = slice_stat(efile);

  println("input :\n{fptr}", string);

  struct vason_container c = parseStr(local, string);

  println("{vason_container}", c);

  // debugAllocatorDeInit(local);
  // println("arena footprint: {}", arena_footprint(real));
  return 0;
}

#include "../wheels.h"
