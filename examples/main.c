#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../vason.h"

typedef struct vason_container vason_container;
REGISTER_PRINTER(vason_container, {
  vason_object o = in.top;
  switch (o.tag) {
    case vason_ARR:
      PUTS(L"󰅨 ");
      PUTS(L"[");
      for (usize i = 0; i < o.data.list.len; i++) {
        struct vason_object value = o.data.list.array[i];
        typeof(in.top) duplicate = in.top;
        in.top = value;
        USETYPEPRINTER(vason_container, in);
        in.top = duplicate;
        PUTS(L",");
      }
      PUTS(L"]");
      break;
    case vason_MAP:
      PUTS(L"󱃖 ");
      PUTS(L"{");
      for (usize i = 0; i < o.data.object.len; i++) {
        struct vason_string name = o.data.object.names[i];
        struct vason_object value = o.data.object.array[i];
        USETYPEPRINTER(fptr, fptr_fromPL(in.string.ptr + name.offset, name.len));
        PUTS(L":");
        typeof(in.top) duplicate = in.top;
        in.top = value;
        USETYPEPRINTER(vason_container, in);
        in.top = duplicate;
        PUTS(L";");
      }
      PUTS(L"}");
      break;
    case vason_STR:
      PUTS(L"󰅳 ");
      struct vason_string str = o.data.string;
      USETYPEPRINTER(fptr, fptr_fromPL(in.string.ptr + str.offset, str.len));
      break;
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

  println("arena footprint: {}", arena_footprint(local));
  return 0;
}

#include "../wheels.h"
