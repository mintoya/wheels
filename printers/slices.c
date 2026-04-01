#include "../mytypes.h"
#include "../print.h"
#include <stdlib.h>

sliceDef(u8);
REGISTER_SPECIAL_PRINTER_NEEDID(slice_printer_generic_version, "slice", slice(u8), {
  // this will crash if you pass the wrong  type, but so would anything else
  fptr farg = printer_arg_trim(args);
  void *ptr = in.ptr;
  var_ printer = PrinterSingleton_get(farg);
  if (!printer) {
    USETYPEPRINTER(pEsc, ((pEsc){.fg = {.r = 255}, .fgset = 1}));
    PUTS("__could'nt find printer for ");
    USENAMEDPRINTER("slice(c8)", farg);
    PUTS("__");
    USETYPEPRINTER(pEsc, (pEsc){.reset = 1});
  } else {
    PUTC((c8)'[');
    usize size = 0;
    for (each_RANGE(usize, i, 0, in.len)) {
      if (i)
        PUTC((c8)',');
      size = printer(put, size * i + (u8 *)ptr, nullFptr, _arb);
    }
    PUTC((c8)']');
  }
});
