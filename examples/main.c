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

  fptr string = fptr_fromCS(
      (u8 *)("\n"
             "a\n"
             " [ \n"
             "hello , world ,\n"
             " { hello : world ; list : [hello/,world,]; }\n"
             " ]"
             "\n")
  );
  vason_token *t = vason_tokenize(local, string);
  vason_handleEscape(string, t);
  vason_combineStrings(string, t);
  struct vason_result object = vason_parse(local, string, t);
  // nullable(int) i = {};
  // i.data = 1;
  // i.exists = true;
  return 0;
}

#include "../wheels.h"
