#include "../print.h"
#include "../types.h"

REGISTER_SPECIAL_PRINTER_NEEDID(print_c8slice, "slice(c8)", slice(c8), {
  for (each_slice(in, i))
    USETYPEPRINTER(c8, *i);
});
REGISTER_SPECIAL_PRINTER_NEEDID(print_c32slice, "slice(c32)", slice(c32), {
  for (each_slice(in, i))
    USETYPEPRINTER(c32, *i);
});
REGISTER_SPECIAL_PRINTER_NEEDID(print_i32slice, "slice(i32)", slice(i32), {
  PUTS(U"[");
  for (each_slice(in, i)) {
    USETYPEPRINTER(isize, *i);
    PUTS(U",");
  }
  PUTS(U"]");
});
