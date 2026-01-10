#include "../arenaAllocator.h"
#include "../print.h"
//
#include "../kmlast.c"
REGISTER_SPECIAL_PRINTER("vason_obj", vason_object, {
  PUTS(L"unimplemented");
  // switch (in.tag) {
  //   case vason_STR: {
  //     USETYPEPRINTER(fptr, ((fptr){in.value->len, in.value->data.string}));
  //   } break;
  //   case vason_LST: {
  //     PUTC(L'[');
  //     for (u32 i = 0; i < in.value->len; i++) {
  //       USENAMEDPRINTER("vason_obj", in.value->data.items[i]);
  //       if (i < in.value->len - 1) {
  //         PUTC(L',');
  //       }
  //     }
  //     PUTC(L']');
  //   } break;
  //   case vason_MAP: {
  //     PUTC(L'{');
  //     for (u32 i = 0; i < in.value->len; i++) {
  //       vason_object *item = &in.value->data.items[i];
  //       if (item->key) {
  //         USETYPEPRINTER(fptr, ((fptr){item->key->len, item->key->ptr}));
  //         PUTC(L':');
  //       }
  //       USENAMEDPRINTER("vason_obj", *item);
  //       if (i < in.value->len - 1)
  //         PUTC(L';');
  //     }
  //     PUTC(L'}');
  //   } break;
  // }
});
int main(void) {
  Arena_scoped *local = arena_new_ext(defaultAlloc, 4096);

  // slice(u8) string = {
  //     113,
  //     (u8"outside of /[] or /{}, shouldnt be read\n"
  //      "[ \n"
  //      " hello , world ,\n"
  //      " {\n"
  //      "  hello : world; \n"
  //      "  list : [hello/,world,];\n"
  //      " }\n"
  //      "]"
  //      "\n")
  // };
  slice(u8) string = {
      128,
      (u8"outside of /[] or /{}, shouldnt be read\n"
       "{ \n"
       " hello world : world ;\n"
       " 2:{\n"
       "  hello : world; \n"
       "  list : [hello/,world,];\n"
       " };\n"
       "}a: b;"
       "\n")
  };
  println("input :\n{fptr}", string);
  auto t = vason_tokenize(local, string);
  vason_combineStrings(string, t);
  breakdown(t, local, string);

  println("arena footprint: {}", arena_footprint(local));
  return 0;
}

#include "../wheels.h"
