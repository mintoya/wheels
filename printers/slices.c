#include "../print.h"
#include "../types.h"

REGISTER_SPECIAL_PRINTER_NEEDID(c8slice, "slice(c8)", slice(c8), {
  for (each_slice(in, i)) {
    USETYPEPRINTER(char, *i);
  }
});
REGISTER_SPECIAL_PRINTER_NEEDID(i32slice, "slice(i32)", slice(i32), {
  PUTS(L"[");
  for (each_slice(in, i)) {
    USETYPEPRINTER(ssize, *i);
    PUTS(L",");
  }
  PUTS(L"]");
});
