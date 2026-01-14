#include "../vason.h"
#include "../print.h"
typedef struct vason_container vason_container;
REGISTER_PRINTER(vason_container, {
  switch (in.top.tag) {
    case vason_ID:
    case vason_STR: {
      PUTS(L"󰅳 ");
      slice(c8) str = {
          .len = in.top.span.len,
          .ptr = in.string.ptr + in.top.span.offset,
      };
      USENAMEDPRINTER("fptr", str);
    }; break;
    case vason_ARR: {
      PUTS(L"󰅨 ");
      PUTS(L"[");
      for (auto i = 0; i < in.top.span.len; i++) {
        vason_object swap = in.objects.ptr[i + in.top.span.offset];
        vason_object origional = in.top;
        in.top = swap;
        USETYPEPRINTER(vason_container, in);
        in.top = origional;
      }
      PUTS(L"]");
    }; break;
    case vason_MAP: {
      PUTS(L"󱃖 ");
      PUTS(L"{");
      for (auto i = 0; i < in.top.span.len; i += 2) {
        vason_object origional = in.top;

        vason_object name = in.objects.ptr[i + in.top.span.offset];
        vason_object object = in.objects.ptr[i + in.top.span.offset + 1];
        in.top = name;
        USETYPEPRINTER(vason_container, in);
        PUTS(L":");
        in.top = object;
        USETYPEPRINTER(vason_container, in);

        in.top = origional;
      }
      PUTS(L"}");
    }; break;
    case vason_INVALID: {
      PUTS(L"󰅰 ");
    }; break;
  }
});
