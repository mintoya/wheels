#if !defined ESC_PRINTERS_H
  #define ESC_PRINTERS_H (1)
  #include "int_printers.h"
  #include "print_pre.h"
typePrinter(pEsc) {
  if (in.poset) {

    PUTS("\033["); // ]
    USETYPEPRINTER(usize, in.pos.row);
    PUTS(";");
    USETYPEPRINTER(usize, in.pos.col);
    PUTS("H");
  }
  if (in.fgset) {
    PUTS("\033[38;2;"); // ]
    USETYPEPRINTER(usize, in.fg.r);
    PUTS(";");
    USETYPEPRINTER(usize, in.fg.g);
    PUTS(";");
    USETYPEPRINTER(usize, in.fg.b);
    PUTS("m");
  }

  if (in.bgset) {
    PUTS("\033[48;2;"); // ]
    USETYPEPRINTER(usize, in.bg.r);
    PUTS(";");
    USETYPEPRINTER(usize, in.bg.g);
    PUTS(";");
    USETYPEPRINTER(usize, in.bg.b);
    PUTS("m");
  }
  if (in.clear) {
    PUTS("\033[2J"); // ]
    PUTS("\033[H");  // ]
  }
  if (in.reset) {
    PUTS("\033[0m"); // ]
  }
}
#endif
