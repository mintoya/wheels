#if !defined INTEGER_PRITNERS_H
  #define INTEGER_PRITNERS_H
  #include "../mytypes.h"
  #include "print_pre.h"
typePrinter(u64) {
  c8 digits[sizeof(u64) * 8 / 3];
  u8 digit = 0;
  usize l = 1;
  while (l <= in / 10) {
    if (l * 10 < l) break;
    else l = l * 10;
  }
  while (l) {
    digits[digit++] = in / l + '0';
    in %= l;
    l /= 10;
  }
  PUTS(*VLAP(digits, digit + 1));
}
typePrinter(i64) {
  usize uin = (usize)in;
  if (in < 0) {
    PUTC((c8)'-');
    uin = 0 - in;
  }
  USETYPEPRINTER(u64, uin);
}
typePrinter(usize) { USETYPEPRINTER(u64, (u64)in); }
typePrinter(isize) { USETYPEPRINTER(i64, (i64)in); }
typePrinter(u8) { USETYPEPRINTER(u64, (u64)in); }
typePrinter(u16) { USETYPEPRINTER(u64, (u64)in); }
typePrinter(u32) { USETYPEPRINTER(u64, (usize)in); }
typePrinter(i8) { USETYPEPRINTER(i64, (i64)in); }
typePrinter(i16) { USETYPEPRINTER(i64, (i64)in); }
typePrinter(i32) { USETYPEPRINTER(i64, (isize)in); }
#endif
