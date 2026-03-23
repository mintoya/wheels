#include "../mytypes.h"
#include "../print.h"
#include <stdlib.h>

sliceDef(c32);
sliceDef(i32);
REGISTER_SPECIAL_PRINTER_NEEDID(print_c8slice, "slice(c8)", slice(c8), {
  foreach (c32 c, slice_vla(in))
    PUTC(c);
});
REGISTER_SPECIAL_PRINTER_NEEDID(print_c32slice, "slice(c32)", slice(c32), {
  foreach (i32 c, slice_vla(in))
    USETYPEPRINTER(i32, c);
});
REGISTER_SPECIAL_PRINTER_NEEDID(print_i32slice, "slice(i32)", slice(i32), {
  PUTS(U"[");
  foreach (i32 i, slice_vla(in))
    PUTC(i);
  PUTS(U"]");
});
