#include "../mytypes.h"
#include "../print.h"
#include <stdlib.h>

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
REGISTER_SPECIAL_PRINTER("slice", fptr, {
  PRINTERARGSEACH({
    PUTS(U"[");
    USETYPEPRINTER(usize, in.width);
    PUTS(U"]");
    USETYPEPRINTER(fptr, arg);
    // char *conName = (typeof(conName))malloc(sizeof("slice()") + arg.width);
    // free(conName);
  });
});
