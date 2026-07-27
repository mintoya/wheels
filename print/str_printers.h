#if !defined STR_PRINTERS_H
  #define STR_PRINTERS_H (1)
  #include "escape_printers.h"
typePrinter("slice(c8)", slice(c8)) {
  foreach (c8 *c, span(in.ptr, in.len))
    PUTC(*c);
}
typePrinter(c8) { PUTC(in); }
typePrinter(cstr) {
  let orange = (pEsc){.fg = {255, 128, 64}, .fgset = true};
  let reset = (pEsc){.reset = true};
  if (in) {
    PUTS("(");
    USETYPEPRINTER(pEsc, orange);
    PUTS("null");
    USETYPEPRINTER(pEsc, reset);
    PUTS(")");
  } else PUTS(*VLAP(in, strlen(in)));
}

static void GETTYPEPRINTERFN(carr)(fptr _v_in_ptr, printerfunction_context _ctx) {
  PUTS(*VLAP((char *)_v_in_ptr.ptr, _v_in_ptr.len));
}
__attribute__((constructor(203))) static void printerConstructor_carr() {
  PrinterSingleton_append(fp("carr"), (printerFunction){GETTYPEPRINTERFN(carr), ~(usize)0});
}

typePrinter(c32) {
  if (in <= 0x7F)
    PUTC((c8)in);
  else if (in <= 0x7FF) {
    PUTC((c8)(0xC0 | (in >> 6)));
    PUTC((c8)(0x80 | (in & 0x3F)));
  } else if (in <= 0xFFFF) {
    PUTC((c8)(0xE0 | (in >> 12)));
    PUTC((c8)(0x80 | ((in >> 6) & 0x3F)));
    PUTC((c8)(0x80 | (in & 0x3F)));
  } else {
    PUTC((c8)(0xF0 | (in >> 18)));
    PUTC((c8)(0x80 | ((in >> 12) & 0x3F)));
    PUTC((c8)(0x80 | ((in >> 6) & 0x3F)));
    PUTC((c8)(0x80 | (in & 0x3F)));
  }
}
typePrinter("c32str", c32 *) {
  if (in)
    while (*in)
      USETYPEPRINTER(c32, *in++);
  else
    PUTS("__NULLCSTR__");
}
#endif
