#include "../mytypes.h"
#include "../print.h"
#include <stdlib.h>

sliceDef(c32);
sliceDef(i32);
REGISTER_SPECIAL_PRINTER_NEEDID(print_c8slice, "slice(c8)", slice(c8), {
  for_each((c32 c, slice_vla(in)), {
    PUTC(c);
  });
});
REGISTER_SPECIAL_PRINTER_NEEDID(print_c32slice, "slice(c32)", slice(c32), {
  for_each((i32 c, slice_vla(in)), {
    USETYPEPRINTER(i32, c);
  });
});
REGISTER_SPECIAL_PRINTER_NEEDID(print_i32slice, "slice(i32)", slice(i32), {
  PUTC('[');
  for_each((i32 i, slice_vla(in)), {
    USETYPEPRINTER(i32, i);
  });
  PUTC(']');
});
